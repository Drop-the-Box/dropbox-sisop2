#include <cstring>
#include <fstream>
#include <iostream>
#include <plog/Log.h>
#include <pthread.h>
#include <regex>
#include <set>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>

#include "../../common/eventhub/models.hpp"
#include "../../common/vars.hpp"
#include "../eventhub/file_sync.hpp"
#include "../eventhub/publisher.hpp"
#include "../eventhub/subscriber.hpp"
#include "../userland/models.hpp"
#include "../serverland/replication.hpp"
#include "session.hpp"
#include <memory>
#include <thread>
#include <functional>
#include <chrono>

using namespace std;

void *run_server_sync(void *election_svc_ptr) {
    shared_ptr<ServerElectionService> election_service((ServerElectionService *) election_svc_ptr);
    PLOGD << "Starting server sync..." << endl;
    election_service->sync_servers();
    return NULL;
}


SessionManager::SessionManager(shared_ptr<Socket> socket, shared_ptr<ServerElectionService> election_svc) {
    this->socket      = socket;
    this->num_threads = 0;
    this->election_svc = election_svc;
}

void SessionManager::start(int pid) {
    char *client_addr = (char *)calloc(INET_ADDRSTRLEN, sizeof(uint8_t));
    int  *client_port = (int *)calloc(1, sizeof(int));
    shared_ptr<UserStore> storage(new UserStore());
    set<int> channels;
    pthread_t srv_sync_id;
    pthread_create(&srv_sync_id, NULL, run_server_sync, this->election_svc.get());
    PLOGI << "Waiting for connections...\n\n";
    while (!*socket->interrupt) {
        int channel = this->socket->accept(client_addr, client_port);
        if (channel != -1) {
            PLOGD << "Connected to " << client_addr << ":" << *client_port << " on channel " << channel << endl;
            channels.insert(channel);
            this->num_threads += 1;
            PLOGD << "Creating new Connection ..." << endl;
            int pipe_fd[2];
            pipe(pipe_fd);
            shared_ptr<Connection> connection(new Connection(client_addr, *client_port, channel, pipe_fd));

            connection->get_conection_info();
            
            shared_ptr<ServerContext> context(
                new ServerContext(socket, connection, storage, election_svc)
            );
            this->create_session(channel, context);
        }
        usleep(10000);
    }
    for (set<int>::iterator channel = channels.begin(); channel != channels.end(); channel++) {
        PLOGD << "Closing socket on channel " << *channel << "..." << endl;
        socket->close(*channel);
        pthread_t thread_id = this->thread_pool.at(*channel);
        PLOGD << "Waiting for thread " << thread_id << " from channel " << *channel << " to teardown..." << endl;
        pthread_join(thread_id, NULL);
    }
    free(client_addr);
}

void SessionManager::create_session(int channel, shared_ptr<ServerContext> context) {
    PLOGD << "Spawning new connection on channel " << channel << endl;
    shared_ptr<Connection> connection = context->connection;
    connection->set_thread_id(&thread_pool[num_threads]);
    Session *session = new Session(context);
    pthread_create(&thread_pool[num_threads], NULL, this->handle_session, session);
}

void log_thread(string message) {
}

void *SessionManager::handle_session(void *session_ptr) {
    shared_ptr<Session> session((Session *)session_ptr);
    int                 channel = session->context->connection->channel;

    PLOGD << "Running new thread for channel " << channel << endl;
    string client_addr = session->context->connection->get_full_address();
    PLOGD << "Connected to " << client_addr << " on channel " << channel << endl;

    if (channel == -1) {
        PLOGE << "Error: Invalid channel: " << channel << endl;
        ::pthread_exit(NULL);
    }
    PLOGD << "Listening on channel " << channel << endl;

    if (session->setup()) {
        PLOGD << "Starting session on channel " << channel << "..." << endl;
        session->run();
        PLOGD << "Session on channel " << channel << " finished." << endl;
    }

    PLOGD << "Closing channel " << channel << "..." << endl;
    session->teardown();
    ::pthread_exit(NULL);
    return NULL;
}

void Session::handle_server_elected(shared_ptr<Packet> received_packet) {
    uint16_t pid_bytes;
    memmove(&pid_bytes, received_packet->payload, sizeof(uint16_t));
    int received_pid = (int)ntohs(pid_bytes);

    PLOGD << "Server with PID " << received_pid << " claimed the leadership. Setting it as leader." << endl; 
    this->context->election_service->update_leader(received_pid);
    ostringstream reply;
    reply << "Leader is " << received_pid << endl;
    shared_ptr<Event> reply_evt(new Event(LeaderAccepted, reply.str()));
    reply_evt->send(context->socket, context->connection->channel);
}

void Session::handle_server_election(shared_ptr<Packet> received_packet) {
    uint16_t pid_bytes;
    memmove(&pid_bytes, received_packet->payload, sizeof(uint16_t));
    int received_pid = (int)ntohs(pid_bytes);

    PLOGD << "Received election request from server with PID " << received_pid << endl; 
    string reply = "Ok";
    shared_ptr<Event> reply_evt(new Event(ServerAlive, reply));
    reply_evt->send(context->socket, context->connection->channel);
    shared_ptr<ServerElectionService> election_svc = context->election_service;
    election_svc->start_election();
}

void Session::handle_server_probe(shared_ptr<Packet> received_packet){
    uint16_t pid_bytes;
    memmove(&pid_bytes, received_packet->payload, sizeof(uint16_t));
    int received_pid = (int)ntohs(pid_bytes);

    if (received_pid == 0) {
        PLOGD << "Received probe from client...";
    } else {
        PLOGD << "Received probe from server with PID " << received_pid << endl; 
    }
    string message;
    shared_ptr<ReplicaManager> current_server = this->context->election_service->current_server;
    if (current_server->is_leader) {
        message = "leader";
    } else {
        message = "backup";
    }
    shared_ptr<Event> reply_evt(new Event(ServerStatus, message));
    reply_evt->send(context->socket, context->connection->channel);
}

bool Session::setup() {
    PLOGD << "Setting up session..." << endl;
    unique_ptr<uint8_t> buffer((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t)));
    int                 channel      = context->connection->channel;
    int                 payload_size = context->socket->get_message_sync(buffer.get(), channel);
    if (payload_size == 0) {
        PLOGW << "Client " << context->connection->get_full_address() << " has disconnected before setup.." << endl;
        return false;
    }

    shared_ptr<Packet> packet(new Packet(buffer.get()));

    switch(packet->type) {
        case ClientSessionInit:
            return this->setup_client(packet);
        case ServerSessionInit:
            return this->setup_server(packet);
        case ServerProbeEvent:
            this->handle_server_probe(packet);
            return false;
        case ServerElectionEvent:
            this->handle_server_election(packet);
            break;
        case ServerElectedEvent:
            this->handle_server_elected(packet);
            return false;
        default:
            return false;
    };
    return false;
}



bool Session::setup_server(shared_ptr<Packet> packet) {
    shared_ptr<SessionRequest> request(new SessionRequest(packet->payload));
    shared_ptr<Connection> connection = this->context->connection;
    if (request->type == Unknown) {
        PLOGW << "Invalid session from server " << connection->get_full_address() << endl;
        return false;
    }
    return true;
}


bool Session::setup_client(shared_ptr<Packet> packet) {

    shared_ptr<SessionRequest> request(new SessionRequest(packet->payload));
    // if (!regex_match(string(request->username), regex("[A-Za-z0-9_-]+"))) {
    //     PLOGI << "Invalid username: " << request->username << endl;
    //     return false;
    // }
    shared_ptr<Connection> connection = this->context->connection;
    shared_ptr<ReplicaManager> current_server = this->context->election_service->current_server;
    if (!current_server->is_leader) {
        string client_address = connection->get_full_address();
        PLOGW << "Client connection from " << client_address << " denied as this server is not a leader.";
    }
    if (request->type == Unknown) {
        PLOGW << "Invalid session from device " << connection->get_full_address() << " and user " << request->username << endl;
        return false;
    }

    PLOGD << "Established session with user " << request->username << endl;
    shared_ptr<UserStore> storage = this->context->storage;
    if (!storage->register_connection(request->username, context)) {
        PLOGW << "Cannot establish connection for user " << request->username
              << " on device " << connection->get_full_address() << endl;
        return false;
    }
    this->type = request->type;
    string session_type;
    try {
        session_type = session_type_map.at(request->type);
    } catch (const std::exception &exc) {
        PLOGE << "Cannot create session of type: " << request->type << endl;
        return false;
    }
    ostringstream oss;
    oss << "Session of type " << session_type << " established." << endl;
    PLOGD << oss.str();
    shared_ptr<Event> accept_evt(new Event(SessionAccepted, oss.str()));
    accept_evt->send(context->socket, context->connection->channel);
    return true;
}

Session::Session(shared_ptr<ServerContext> context) {
    this->context = context;
}

void Session::run() {
    string session_type;
    try {
        session_type = session_type_map.at(this->type);
        PLOGW << "Running session of type " << session_type << endl;
    } catch (const exception &exc) {
        // blabla
    }
    switch (type) {
    case FileExchange: {
        unique_ptr<FileSync> file_sync(new FileSync(this->context));
        file_sync->loop();
        break;
    }
    case CommandPublisher: {
        unique_ptr<ServerEventPublisher> publisher(new ServerEventPublisher(this->context));
        publisher->loop();
        break;
    }
    case CommandSubscriber: {
        unique_ptr<ServerEventSubscriber> subscriber(new ServerEventSubscriber(this->context));
        subscriber->loop();
        break;
    }
    default: {
        PLOGE << "Invalid session type: " << this->type << endl;
        return;
    }
    }
};

void Session::teardown() {
    int    channel      = context->connection->channel;
    string full_address = context->connection->get_full_address();
    if (context->device != NULL) {
        string username = context->device->username;
        PLOGD << "Unregistering connection from device " << full_address << " from user " << username << endl;
        context->storage->unregister_connection(context);
    } else {
        PLOGD << "Unregistering unauthenticated connection from device " << full_address << endl;
    }
    context->socket->close(channel);
}
