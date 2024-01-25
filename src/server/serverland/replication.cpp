#include "replication.hpp"
#include "../../common/vars.hpp"
#include "../../common/eventhub/models.hpp"

bool ServerStore::has_server(int pid) {
    return this->replica_managers.find(pid) != this->replica_managers.end();
}

void ServerStore::add_server(int pid, shared_ptr<ReplicaManager> server) {
    this->replica_managers[pid] = server; 
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


ServerElectionService::ServerElectionService(
    shared_ptr<ReplicaManager> current_server, shared_ptr<ServerStore> server_store, bool *interrupt_ptr
) {
    this->current_server = current_server;
    this->server_store = server_store;
    this->interrupt = interrupt_ptr;
}

void ServerElectionService::sync_servers() {
    server_params server_list[5] = {
        {1, 6999, "dtb-server-1"},
        {2, 6999, "dtb-server-2"},
        {3, 6999, "dtb-server-3"},
        {4, 6999, "dtb-server-4"},
        {5, 6999, "dtb-server-5"},
    };
    int current_pid = this->current_server->server_pid;

    uint16_t server_pid = htons(current_pid);
    uint16_t sreq_size = sizeof(uint16_t);
    uint8_t *enc_data = (uint8_t *)malloc(sreq_size);
    memmove(enc_data, &server_pid, sreq_size);
    unique_ptr<Packet> packet(new Packet(ServerProbeEvent, 1, sreq_size, sreq_size, enc_data));

    shared_ptr<uint8_t> msg((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t))); 

    while(!*this->interrupt) {
        bool bully = !this->current_server->is_leader;
        
        for (int idx = 0; idx < 5; idx++) {
            server_params server = server_list[idx];
            if (server.pid == current_pid) {
                continue;
            }
            shared_ptr<ReplicaManager> new_replica(
                new ReplicaManager(server.pid, server.address, server.port)
            );
            try {
                shared_ptr<Socket> socket = get_peer_socket(new_replica, this->interrupt);
                packet->send(socket, socket->socket_fd);
                PLOGD << "Sending probe message to server " << server.pid << endl;
                socket->get_message_sync(msg.get(), socket->socket_fd, true);
            } catch(const std::exception& exc) {
                PLOGW << "Server " << server.pid << " not accessible." << endl;
                continue;
            }
            unique_ptr<Packet> resp_packet(new Packet(msg.get()));
            if (resp_packet->type == EventMsg) {
                unique_ptr<Event> evt(new Event(resp_packet->payload));
                PLOGD << "Reply from server with PID " << server.pid << ": " << evt->message << endl;
                string leader_msg = "leader";
                bool is_leader = false;
                if (evt->type == ServerStatus && leader_msg.compare(evt->message) == 0) {
                    if (server.pid > current_pid) {
                        is_leader = true;
                        bully = false;
                        PLOGD << "Server " << server.pid << " is already a leader." << endl;
                    }
                }
                if (!server_store->has_server(server.pid)) {
                    this->server_store->add_server(server.pid, new_replica);
                } 
                if(is_leader) {
                    this->server_store->set_leader(server.pid);
                }
            }
        }
        if (bully) {
            PLOGW << "Lower PID or none server elected. Starting bullying election process..." << endl;
            this->start_election();
            // Start election process
        }
        sleep(60);
    }
}

void ServerElectionService::start_election() {
    map<int, shared_ptr<ReplicaManager>> replicas = server_store->replica_managers; 
    map<int, shared_ptr<ReplicaManager>>::iterator iter;

    bool elect_self = true;
    int current_pid = this->current_server->server_pid;

    uint16_t server_pid = htons(current_pid);
    uint16_t sreq_size = sizeof(uint16_t);
    uint8_t *enc_data = (uint8_t *)malloc(sreq_size);
    memmove(enc_data, &server_pid, sreq_size);
    unique_ptr<Packet> packet(new Packet(ServerElectionEvent, 1, sreq_size, sreq_size, enc_data));

    shared_ptr<uint8_t> msg((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t))); 
    string ok_reply = "Ok";
    

    for(iter = replicas.begin(); iter != replicas.end(); iter++) {
        if (iter->second->server_pid < current_pid) {
            continue;
        }
        shared_ptr<ReplicaManager> server = iter->second;
        try {
            shared_ptr<Socket> socket = get_peer_socket(server, this->interrupt);
            packet->send(socket, socket->socket_fd);
            PLOGD << "Sending election message to server " << server->server_pid << endl;
            socket->get_message_sync(msg.get(), socket->socket_fd, true);
        } catch(const std::exception& exc) {
            PLOGW << "Server " << server->server_pid << " not accessible.";
            continue;
        }
        unique_ptr<Packet> resp_packet(new Packet(msg.get()));
        if (resp_packet->type == EventMsg) {
            unique_ptr<Event> evt(new Event(resp_packet->payload));
            PLOGD << "Reply from server with PID " << server_pid << ": " << evt->message << endl;
            if (evt->type == ServerAlive) {
                PLOGI << "Server " << server->server_pid << " with higher priority will continue the election." << endl;
                elect_self = false;
                break;
            }
        }

    }
    if (elect_self) {
        // Notify everyone, I'm the leader
        this->current_server->is_leader = true;
        this->notify_elected();
        PLOGI << "Server " << current_pid << " is the new leader.";
    }
}

void ServerElectionService::notify_elected() {
    map<int, shared_ptr<ReplicaManager>> replicas = server_store->replica_managers; 
    map<int, shared_ptr<ReplicaManager>>::iterator iter;
    
    int server_pid = this->current_server->server_pid;
    uint16_t enc_pid = htons(server_pid);
    uint16_t sreq_size = sizeof(uint16_t);
    uint8_t *enc_data = (uint8_t *)malloc(sreq_size);
    memmove(enc_data, &enc_pid, sreq_size);
    unique_ptr<Packet> packet(new Packet(ServerElectedEvent, 1, sreq_size, sreq_size, enc_data));
    
    shared_ptr<uint8_t> msg((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t))); 

    for(iter = replicas.begin(); iter != replicas.end(); iter++) {
        shared_ptr<ReplicaManager> server = iter->second;
        try {
            shared_ptr<Socket> socket = get_peer_socket(server, this->interrupt);
            packet->send(socket, socket->socket_fd);
            PLOGD << "Sending elected message to server " << server->server_pid << endl;
            socket->get_message_sync(msg.get(), socket->socket_fd, true);
        } catch(const std::exception& exc) {
            PLOGW << "Server " << server->server_pid << " not accessible.";
            continue;
        }
        unique_ptr<Packet> resp_packet(new Packet(msg.get()));
        if (resp_packet->type == EventMsg) {
            unique_ptr<Event> evt(new Event(resp_packet->payload));
            if (evt->type == LeaderAccepted) {
                PLOGD << "Server " << server->server_pid << " accepted the leader: " << evt->message << endl;
            }
        }
    }
}

void ServerElectionService::update_leader(int leader_pid) {
    this->current_server->is_leader = false;
    this->server_store->set_leader(leader_pid);
}
