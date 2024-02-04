#include <plog/Log.h>
#include <memory>
#include <pthread.h>
#include "../../common/eventhub/models.hpp"
#include "../../common/vars.hpp"
#include "../../common/session/models.hpp"
#include "conn_manager.hpp"


ConnectionManager::ConnectionManager(string username, bool *interrupt) {
    this->username = username;
    this->interrupt = interrupt;
    this->init_connections();

    pthread_mutex_init(&reconnect_mutex, NULL);
}

ConnectionManager::~ConnectionManager() {
    this->stop_connections();
}

void ConnectionManager::stop_connections() {
    pthread_mutex_destroy(&reconnect_mutex);
    map<SessionType, shared_ptr<Socket>>::iterator iter;
    for(iter=socket_map.begin(); iter != socket_map.end(); iter++) {
        PLOGI << "Stopping client socket on channel " << iter->second->socket_fd << endl;
        iter->second->close(iter->second->socket_fd);  // close current socket  
        iter->second->socket_fd = -1;
    }
    this->disconnecting = true;
}

void ConnectionManager::init_connections() {
    bool connected = false;
    shared_ptr<Socket> publisher_socket;
    shared_ptr<Socket> subscriber_socket;
    shared_ptr<Socket> file_sync_socket;
    while (!connected) {
        shared_ptr<ReplicaManager> leader = this->get_server_leader();
        if (leader == NULL) {
            PLOGE << "No leaders were found.. Will continue after 5 seconds...";
            sleep(5);
            continue;
        }
        publisher_socket = get_peer_socket(leader, this->interrupt);
        connected = this->setup_connection(publisher_socket, CommandPublisher);
        subscriber_socket = get_peer_socket(leader, this->interrupt);
        connected &= this->setup_connection(subscriber_socket, CommandSubscriber);
        file_sync_socket = get_peer_socket(leader, this->interrupt);
        connected &= this->setup_connection(file_sync_socket, FileExchange);
    }
    this->socket_map[CommandPublisher] = publisher_socket;
    this->socket_map[CommandSubscriber] = subscriber_socket;
    this->socket_map[FileExchange] = file_sync_socket;
    PLOGI << "Created new publisher socket in channel " << publisher_socket->socket_fd << endl;
    PLOGI << "Created new subscriber socket in channel " << subscriber_socket->socket_fd << endl;
    PLOGI << "Created new filesync socket in channel " << file_sync_socket->socket_fd << endl;
}

bool ConnectionManager::setup_connection(shared_ptr<Socket> socket, SessionType kind) {
    shared_ptr<SessionRequest> request(new SessionRequest(kind, this->username));
    uint8_t           *bytes;
    uint8_t          **bytes_ptr = &bytes;
    size_t             sreq_size = request->to_bytes(bytes_ptr);
    shared_ptr<Packet> packet(new Packet(ClientSessionInit, 1, sreq_size, sreq_size, bytes));
    int bytes_sent = 0;
    try {
        bytes_sent = packet->send(socket, socket->socket_fd);
    } catch (SocketError &exc) {
        return false;
    }
    if (bytes_sent < 0) {
        return false;
    }

    shared_ptr<uint8_t> msg((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t))); 
    try {
        socket->get_message_sync(msg.get(), socket->socket_fd, true);
    } catch (SocketError &exc) {
        return false;
    }

    PLOGD << "Server returned: " << msg.get() << endl;
    unique_ptr<Packet> resp_packet(new Packet(msg.get()));
    if (resp_packet->type == EventMsg) {
        unique_ptr<Event> evt(new Event(resp_packet->payload));
        if (evt->type == SessionAccepted) {
            PLOGI << "Session accepted from server..." << endl;
            return true;
        }
        PLOGD << "Event detail: " << evt->message << endl;
    }
    free(bytes);
    return false;
}


shared_ptr<Socket> ConnectionManager::get_socket(SessionType kind) {
    shared_ptr<Socket> socket = NULL;
    while(socket == NULL || this->reconnecting) {
        socket = this->socket_map.at(kind);
    }
    return socket;
}

bool ConnectionManager::has_error(SessionType kind) {
    bool result = false;
    shared_ptr<Socket> socket = this->get_socket(kind);
    try { 
        return socket->has_error(socket->socket_fd);
    } catch (SocketError &exc) {
        try {
            this->reconnect();   
        } catch (ConnectionResetError) {};
    }
    return result;
}

void ConnectionManager::reconnect() {
    if(pthread_mutex_trylock(&reconnect_mutex) == 0) {
        PLOGI << "Reconnecting sockets with new leader..." << endl;
        reconnecting = true;
        map<SessionType, shared_ptr<Socket>>::iterator iter;
        for(iter=socket_map.begin(); iter != socket_map.end(); iter++) {
            PLOGI << "Closing client channel " << iter->first << endl;
            shared_ptr<Socket> socket = iter->second;
            socket->close(socket->socket_fd);  // close current socket
            socket->socket_fd = -1;
            socket_map[iter->first] = NULL;
        }
        this->init_connections();
        reconnecting = false;
        pthread_mutex_unlock(&reconnect_mutex);
    } else {
        while(this->reconnecting) {
            PLOGD << "Awaiting reconnection by another thread" << endl;
        }
    }
    throw ConnectionResetError("Operation Failed. Please try again.");
}

// void ConnectionManager::check_socket(SessionType kind) {
//     shared_ptr<Socket> socket = this->socket_map[kind];
//     if (!this->probe_server(socket)) {
//         PLOGI << "Finding a new server leader..." << endl;
//         socket->close(socket->socket_fd);
//         shared_ptr<ReplicaManager> new_leader = this->get_server_leader();
//         PLOGI << "New leader is with PID " << new_leader->server_pid << endl;
//         socket = get_peer_socket(new_leader, this->interrupt);
//         this->socket_map[kind] = socket;
//     }
// }

bool ConnectionManager::probe_server(shared_ptr<Socket> socket, int pid) {
    uint16_t server_pid = htons(0);
    uint16_t sreq_size = sizeof(uint16_t);
    uint8_t *enc_data = (uint8_t *)malloc(sreq_size);
    memmove(enc_data, &server_pid, sreq_size);
    unique_ptr<Packet> packet(new Packet(ServerProbeEvent, 1, sreq_size, sreq_size, enc_data));
    shared_ptr<uint8_t> msg((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t))); 

    try {
        packet->send(socket, socket->socket_fd);
        PLOGD << "Sending probe message to server " << pid << endl;
        socket->get_message_sync(msg.get(), socket->socket_fd, true);
    } catch (std::exception &exc){
        PLOGW << "Server " << pid << " is down. Detail: " << exc.what() << endl;
    }
    unique_ptr<Packet> resp_packet(new Packet(msg.get()));
    if (resp_packet->type == EventMsg) {
        unique_ptr<Event> evt(new Event(resp_packet->payload));
        string leader_msg = "leader";
        if (evt->type == ServerStatus && leader_msg.compare(evt->message) == 0) {
            PLOGI << "Server " << pid << " is alive and it is a leader." << evt->message << endl;
            return true;
        } else {
            PLOGW << "Server " << pid << " is alive but not a leader. Returned " << evt->message << endl;
        }
    }
    return false;
}


int ConnectionManager::send(shared_ptr<Packet> packet, SessionType kind) {
    shared_ptr<Socket> socket = this->get_socket(kind);
    try {
        return packet->send(socket, socket->socket_fd);
    } catch (SocketError &err) {
        if (disconnecting) {
            throw UserInterruptError("Disconnecting...");
        }
        PLOGW << "Server disconnected while sending package. Reconnecting..." << endl;
        this->reconnect();
    }
    return 0;
}

int ConnectionManager::get_message(uint8_t *buffer, SessionType kind) {
    shared_ptr<Socket> socket = this->get_socket(kind);
    try {
        return socket->get_message_sync(buffer, socket->socket_fd, true); 
    } catch (SocketTimeoutError &exc) {
        throw exc;
    } catch(SocketError &exc) {
        if (disconnecting) {
            throw UserInterruptError("Disconnecting...");
        }
        PLOGW << "Server disconnected while receiving package. Reconnecting..." << endl;
        this->reconnect();
    }
    return 0;
}

shared_ptr<ReplicaManager> ConnectionManager::get_server_leader() {
    server_params server_list[5] = {
        {1, 6999, "dtb-server-1"},
        {2, 6999, "dtb-server-2"},
        {3, 6999, "dtb-server-3"},
        {4, 6999, "dtb-server-4"},
        {5, 6999, "dtb-server-5"},
    };

    for (int idx = 0; idx < sizeof(server_list)/sizeof(server_params); idx++) {
        server_params server = server_list[idx];
        shared_ptr<ReplicaManager> replica(
            new ReplicaManager(server.pid, server.address, server.port)
        );
        bool is_leader = false;
        try {
            shared_ptr<Socket> socket = get_peer_socket(replica, this->interrupt);
            is_leader = this->probe_server(socket, server.pid);
        } catch(const std::exception& exc) {
            PLOGW << "Server " << server.pid << " is down." << endl;
            continue;
        }
        if (is_leader) {
            replica->is_leader = true;
            return replica;
        }
    }
    return NULL;
}


bool ConnectionManager::send_command(shared_ptr<Command> command, SessionType kind) {
    try {
        shared_ptr<Socket> socket = this->get_socket(kind);
        return command->send(socket, socket->socket_fd);
    } catch (SocketError &err) {
        if (disconnecting) {
            throw UserInterruptError("Disconnecting...");
        }
        PLOGW << "Server disconnected while sending package: "
            << err.what() << " Reconnecting..." << endl;
        this->reconnect();
    }
    return false;
}
