#include "replication.hpp"
#include "../../common/vars.hpp"
#include "../../common/eventhub/models.hpp"
#include "../../common/file_io/file_io.hpp"
#include <toml/toml.hpp>


ServerStore::ServerStore(shared_ptr<ReplicaManager> current_server) {
    toml::table server_tbl = toml::parse_file("server_configs.toml");
    this->current_server = current_server;

    for (auto it = server_tbl.begin(); it != server_tbl.end(); it++) {
        auto server = it->second.as_table();
        int pid = *server->at_path("pid").value<int>();
        string address = *server->at_path("address").value<string>();
        int port = *server->at_path("port").value<int>();
        bool is_leader = *server->at_path("leader").value<bool>();
        string base_dir = *server->at_path("base_dir").value<string>();

        if (pid == current_server->server_pid) {
            this->current_server->address = address;
            this->current_server->port = port;
            this->current_server->is_leader = is_leader;
            this->current_server->base_dir = base_dir;
            continue;
        }

        shared_ptr<ReplicaManager> new_server(
            new ReplicaManager(pid, address, port, base_dir, is_leader)
        );
        this->all_servers[pid] = new_server;
    }
}

bool ServerStore::has_server(int pid) {
    return this->replica_managers.find(pid) != this->replica_managers.end();
}

void ServerStore::add_server(int pid, shared_ptr<ReplicaManager> server) {
    this->replica_managers[pid] = server; 
}

void ServerStore::remove_server(int pid) {
    if (replica_managers.find(pid) != replica_managers.end()) {
        this->replica_managers.erase(pid);
    }
}

void ServerStore::set_leader(int pid) {
    map<int, shared_ptr<ReplicaManager>>::iterator server;
    for (server=replica_managers.begin(); server != replica_managers.end(); server++) {
        if (server->second->server_pid == pid) {
            server->second->is_leader = true;
            continue;
        }
        server->second->is_leader = false;
    }
}

ReplicationService::ReplicationService(shared_ptr<ServerStore> storage, bool *interrupt_ptr) {
    this->server_store = storage;
    this->interrupt = interrupt_ptr;
    pthread_mutex_init(&backup_mutex, NULL);
}

void ReplicationService::connect_backup(shared_ptr<ReplicaManager> server_rm) {
    int current_pid = this->server_store->current_server->server_pid;
    int server_pid = server_rm->server_pid;
    if (server_pid > current_pid || server_socket_map.find(server_pid) != server_socket_map.end()) {
        return;
    }
    uint16_t enc_pid = htons(current_pid);
    uint16_t sreq_size = sizeof(uint16_t);
    shared_ptr<uint8_t> enc_data((uint8_t *)malloc(sreq_size));
    memmove(enc_data.get(), &enc_pid, sreq_size);

    shared_ptr<uint8_t> msg((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t))); 
    unique_ptr<Packet> packet(new Packet(ServerSessionInit, 1, sreq_size, sreq_size, enc_data.get()));

    shared_ptr<Socket> back_socket = get_peer_socket(server_rm, this->interrupt);

    PLOGI << "Connecting to backup server " << server_pid << endl;
    try {
        packet->send(back_socket, back_socket->socket_fd);

        PLOGI << "Waiting for backup server " << server_pid << " reply";
        back_socket->get_message_sync(msg.get(), back_socket->socket_fd, true);
    } catch(SocketError &exc) {
        PLOGW << "Error detected while trying to connect backup: " << exc.what() << endl;
        return;
    }

    unique_ptr<Packet> resp_packet(new Packet(msg.get()));
    if (resp_packet->type == EventMsg) {
        unique_ptr<Event> evt(new Event(resp_packet->payload));
        if (evt->type == SessionAccepted) {
            PLOGI << "Session accepted from backup server " << server_pid << endl;
            this->server_socket_map[server_pid] = back_socket;
        }
    }
    memset(msg.get(), 0, BUFFER_SIZE);
}

bool ReplicationService::has_backup(int pid) {
    return server_socket_map.find(pid) != server_socket_map.end();
}

void ReplicationService::connect_backups() {
    pthread_mutex_lock(&backup_mutex);

    int current_pid = this->server_store->current_server->server_pid;
    uint16_t server_pid = htons(current_pid);
    uint16_t sreq_size = sizeof(uint16_t);
    shared_ptr<uint8_t> enc_data((uint8_t *)malloc(sreq_size));
    memmove(enc_data.get(), &server_pid, sreq_size);

    shared_ptr<uint8_t> msg((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t))); 
    unique_ptr<Packet> packet(new Packet(ServerSessionInit, 1, sreq_size, sreq_size, enc_data.get()));
    shared_ptr<Socket> back_socket;

    auto replicas = &this->server_store->replica_managers;
    for (auto it=replicas->begin(); it != replicas->end(); it++) {
        if (server_socket_map.find(it->first) != this->server_socket_map.end()) {
            PLOGI << "Backup " << it->first << " is already connected" << endl;
            continue;
        }
        try {
            this->connect_backup(it->second);
        } catch(const std::exception& exc) {
            PLOGI << "An error hapenned while connecting to server " << it->first << ": " << exc.what() << endl;
            back_socket = NULL;
            // this->server_store->remove_server(it->first);
            continue;
        }
    }
    pthread_mutex_unlock(&backup_mutex);
}

void ReplicationService::disconnect_backups() {
    // close sockets here
    pthread_mutex_lock(&backup_mutex);
    auto replicas = &this->server_socket_map;
    for (auto it=replicas->begin(); it != replicas->end(); it++) {
        PLOGI << "Disconnecting backup server " << it->first << endl;
        shared_ptr<Socket> socket = it->second;
        socket->close(socket->socket_fd);
        socket->socket_fd = -1;
    }
    pthread_mutex_unlock(&backup_mutex);
}

void ReplicationService::send_file(string username, string file) {
    pthread_mutex_lock(&backup_mutex);
    string sync_dir = FileHandler::get_sync_dir(username, DIR_SERVER, this->server_store->current_server);
    unique_ptr<FileHandler> file_handler(new FileHandler(sync_dir));
    string full_file_path = sync_dir + "/" + file;
    PLOGI << "The file " << full_file_path << " was created." << endl;
    ostringstream oss;
    oss << username.c_str() << ' ' << file.c_str();
    string arguments = oss.str();
    unique_ptr<Command> command(new Command(UploadFile, arguments));    
    auto replicas = this->server_socket_map;
    vector<int> to_remove;
    uint8_t buffer[BUFFER_SIZE];
    for (auto it=replicas.begin(); it != replicas.end(); it++) {
        PLOGI << "Sending file to server " << it->first << " with args " << arguments << endl;
        shared_ptr<Socket> socket = it->second;
        file_handler->open(full_file_path);
        try {
            command->send(socket, socket->socket_fd);
            file_handler->send(socket, socket->socket_fd);
            while(true) {
                try {
                    socket->get_message_sync(buffer, socket->socket_fd);
                    break;
                } catch (SocketTimeoutError &exc) {};
            }
            unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
            shared_ptr<Event> event(new Event(packet->payload));
            if (packet->type != EventMsg) {
                return;
            }
            if (event->type == CommandSuccess) {
                PLOGI << "Command has succeeded for delete from server " << it->first << endl;
            } else {
                PLOGI << "Command has failed for delete on server " << it->first
                    << ". Repl: " << event->message << endl; 
            }
        } catch (std::exception &exc) {
            PLOGW << "Error while propagating file to server " << it->first << ": " << exc.what() << endl;
            to_remove.push_back(it->first);
        }
    }
    for (auto it=to_remove.begin(); it != to_remove.end(); it++) {
        replicas.erase(*it);
        this->server_store->remove_server(*it);
    }
    pthread_mutex_unlock(&backup_mutex);
}

void ReplicationService::send_command(string username, shared_ptr<Command> command) {
    pthread_mutex_lock(&backup_mutex);
    ostringstream oss;
    oss << username << " " << command->arguments;
    command->arguments = oss.str();
    auto replicas = this->server_socket_map;
    vector<int> to_remove;
    for (auto it=replicas.begin(); it != replicas.end(); it++) {
        PLOGI << "Sending command to server " << it->first << endl;
        shared_ptr<Socket> socket = it->second;
        try {
            command->send(socket, socket->socket_fd);
        } catch (std::exception &exc) {
            PLOGW << "Error while propagating command to server " << it->first << ": " << exc.what() << endl;
            to_remove.push_back(it->first);
        }
    }
    for (auto it=to_remove.begin(); it != to_remove.end(); it++) {
        replicas.erase(*it);
        this->server_store->remove_server(*it);
    }
    pthread_mutex_unlock(&backup_mutex);
}

ServerElectionService::ServerElectionService(
    shared_ptr<ReplicationService> repl_service, bool *interrupt_ptr
) {
    this->repl_service = repl_service;
    this->server_store = repl_service->server_store;
    this->current_server = repl_service->server_store->current_server;
    this->interrupt = interrupt_ptr;
    pthread_mutex_init(&election_mutex, NULL);
}

ServerElectionService::~ServerElectionService() {
    pthread_mutex_destroy(&election_mutex);
}

string ServerElectionService::get_status() {
    return this->current_server->is_leader ? "leader" : "backup";
}

bool ServerElectionService::probe_server(shared_ptr<ReplicaManager> server_rm) {
    int current_pid = this->current_server->server_pid;
    uint16_t curr_server_pid = htons(current_pid);
    uint16_t sreq_size = sizeof(uint16_t);
    shared_ptr<uint8_t> enc_data((uint8_t *)malloc(sreq_size));
    memmove(enc_data.get(), &curr_server_pid, sreq_size);

    unique_ptr<Packet> packet(new Packet(ServerProbeEvent, 1, sreq_size, sreq_size, enc_data.get()));
    shared_ptr<uint8_t> msg((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t))); 
    int server_pid = server_rm->server_pid;
    shared_ptr<Socket> socket;
    try {
        socket = get_peer_socket(server_rm, this->interrupt);
        packet->send(socket, socket->socket_fd);
        PLOGD << "Sending probe message to server " << server_pid << endl;
    } catch(SocketError &exc) {
        PLOGE << "Cannot create socket for server " << server_pid << ": " << exc.what() << endl;
        return false;
    }
    try {
        return socket->get_message_sync(msg.get(), socket->socket_fd, true);
    } catch (SocketTimeoutError &exc) {
        PLOGW << "Timeout error connecting to " << server_pid << ": " << exc.what();
    } catch(const std::exception& exc) {
        PLOGW << "Server " << server_pid << " not accessible. Reason: " << exc.what() << endl;
        return false;
    }
    unique_ptr<Packet> resp_packet(new Packet(msg.get()));
    if (resp_packet->type == EventMsg) {
        unique_ptr<Event> evt(new Event(resp_packet->payload));
        PLOGD << "Reply from server with PID " << server_rm->server_pid << ": " << evt->message << endl;
        string leader_msg = "leader";
        if (evt->type == ServerStatus && leader_msg.compare(evt->message) == 0) {
            server_rm->is_leader = true;
            PLOGD << "Server " << server_rm->server_pid << " is a leader." << endl;
        }
        return true;
    }
    return false;
}

void ServerElectionService::sync_servers() {
    int current_pid = this->current_server->server_pid;
    shared_ptr<ReplicaManager> leader_rm;

    bool bully = !this->current_server->is_leader;

    auto all_servers = server_store->all_servers;
    for (auto it = all_servers.begin(); it != all_servers.end(); it++) {
        int server_pid = it->first;
        shared_ptr<ReplicaManager> server_rm = it->second;
        if (server_pid == current_pid) {
            continue;
        }
        // if (!this->probe_server(server_rm)) {
        //     continue;
        // }
        if (!server_store->has_server(server_pid)) {
            this->server_store->add_server(server_pid, server_rm);
        } 
        if(server_rm->is_leader) {
            leader_rm = server_rm;
            this->server_store->set_leader(server_pid);
            if (server_rm->server_pid < current_pid) {
                bully = true;
            }
        }
    }
    if (this->current_server->is_leader) {
        // this->notify_elected();
        this->repl_service->connect_backups();
    } else {
        while(!this->probe_server(leader_rm)) {};
    }

    // if (bully && !this->current_server->is_leader) {
    //     PLOGW << "Lower PID or none server elected. Starting bullying election process..." << endl;
    //     this->start_election();
    // }
}

void *ServerElectionService::check_leader(void *svc) {
    PLOGI << "Starting leader probe" << endl;
    ServerElectionService *election_svc = (ServerElectionService *)(svc);
    auto replicas = &election_svc->server_store->replica_managers; 
    shared_ptr<ReplicaManager> leader;
    for(auto iter = replicas->begin(); iter != replicas->end(); iter++) {
        if (iter->second->is_leader) {
            leader = iter->second;
        }
    }
    while(!election_svc->current_server->is_leader && !!leader && leader->is_leader) {
        if (!election_svc->probe_server(leader)) {
            election_svc->start_election();
            PLOGW << "Leader is gone. Starting election process..." << endl;
            break;
        }
        PLOGI << "Leader " << leader->server_pid << " is alive!" << endl;
        sleep(5);
    }
    PLOGI << "Exiting leader probe..." << endl;
    return NULL;
}

void ServerElectionService::start_election() {
    bool elect_self = true;
    int current_pid = this->current_server->server_pid;

    uint16_t server_pid = htons(current_pid);
    uint16_t sreq_size = sizeof(uint16_t);
    uint8_t *enc_data = (uint8_t *)malloc(sreq_size);
    memmove(enc_data, &server_pid, sreq_size);
    unique_ptr<Packet> packet(new Packet(ServerElectionEvent, 1, sreq_size, sreq_size, enc_data));

    shared_ptr<uint8_t> msg((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t))); 
    string ok_reply = "Ok";
    

    auto replicas = &server_store->replica_managers; 
    for(auto iter = replicas->begin(); iter != replicas->end(); iter++) {
        if (iter->second->server_pid < current_pid) {
            continue;
        }
        shared_ptr<ReplicaManager> server = iter->second;
        try {
            shared_ptr<Socket> socket = get_peer_socket(server, this->interrupt);
            packet->send(socket, socket->socket_fd);
            PLOGD << "Sending election message to server " << server->server_pid << endl;
            while(!socket->has_error(socket->socket_fd)) {
                try {
                    socket->get_message_sync(msg.get(), socket->socket_fd, true);
                    break;
                } catch(SocketTimeoutError &exc) {
                    continue;
                }
            }
        } catch(const std::exception& exc) {
            PLOGW << "Server " << server->server_pid << " not accessible: " << exc.what();
            continue;
        }
        unique_ptr<Packet> resp_packet(new Packet(msg.get()));
        if (resp_packet->type == EventMsg) {
            unique_ptr<Event> evt(new Event(resp_packet->payload));
            PLOGD << "Reply from server with PID " << server_pid << ": " << evt->message << endl;
            if (evt->type == ServerAlive) {
                PLOGI << "Server " << server->server_pid << " with higher priority will continue the election." << endl;
                return;
            }
        }

    }
    if (elect_self) {
        // Notify everyone, I'm the leader
        if (this->notify_elected()) {
            this->current_server->is_leader = true;
            this->repl_service->connect_backups();
            PLOGI << "Server " << current_pid << " is the new leader.";
            PLOGI << "Connecting to all backup servers for replication" << endl;
        }
    }
}


bool ServerElectionService::notify_elected_to_backup(shared_ptr<ReplicaManager> server_rm) {
    int server_pid = this->current_server->server_pid;
    uint16_t enc_pid = htons(server_pid);
    uint16_t sreq_size = sizeof(uint16_t);
    uint8_t *enc_data = (uint8_t *)malloc(sreq_size);
    memmove(enc_data, &enc_pid, sreq_size);
    unique_ptr<Packet> packet(new Packet(ServerElectedEvent, 1, sreq_size, sreq_size, enc_data));
    
    shared_ptr<uint8_t> msg((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t))); 
    server_rm->is_leader = false;
    shared_ptr<Socket> socket = get_peer_socket(server_rm, this->interrupt);
    packet->send(socket, socket->socket_fd);
    PLOGD << "Sending elected message to server " << server_rm->server_pid << endl;
    while(!socket->has_error(socket->socket_fd)) {
        try {
            socket->get_message_sync(msg.get(), socket->socket_fd, true);
            break;
        } catch(SocketTimeoutError &exc) {
            continue;
        }
    }
    unique_ptr<Packet> resp_packet(new Packet(msg.get()));
    if (resp_packet->type == EventMsg) {
        unique_ptr<Event> evt(new Event(resp_packet->payload));
        if (evt->type == LeaderAccepted) {
            PLOGD << "Server " << server_rm->server_pid << " accepted the leader: " << evt->message << endl;
        } else if (evt->type == LeaderRejected) {
            PLOGE << "Server " << server_rm->server_pid << " rejected the leader: " << evt->message << endl;
            return false;
        }
    }
    return true;
}

bool ServerElectionService::notify_elected() {
    auto replicas = &server_store->replica_managers; 
    for(auto iter = replicas->begin(); iter != replicas->end(); iter++) {
        shared_ptr<ReplicaManager> server = iter->second;
        try {
            if (!this->notify_elected_to_backup(server)) {
                return false;
            }
        } catch(const std::exception& exc) {
            PLOGW << "Server " << server->server_pid << " not accessible: " << exc.what();
            continue;
        }
    }
    return true;
}

void ServerElectionService::update_leader(int leader_pid) {
    this->current_server->is_leader = false;
    this->server_store->set_leader(leader_pid);
}
