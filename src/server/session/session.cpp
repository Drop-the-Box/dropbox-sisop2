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
#include "session.hpp"

using namespace std;

SessionManager::SessionManager(shared_ptr<Socket> socket) {
    this->socket      = socket;
    this->num_threads = 0;
}

void SessionManager::start() {
    PLOGI << "Waiting for connections...\n\n";
    char                 *client_addr = (char *)calloc(INET_ADDRSTRLEN, sizeof(uint8_t));
    int                  *client_port = (int *)calloc(1, sizeof(int));
    shared_ptr<UserStore> storage(new UserStore());
    set<int>              channels;
    while (true) {
        int channel = this->socket->accept(client_addr, client_port);
        if (channel != -1) {
            PLOGD << "Connected to " << client_addr << ":" << *client_port << " on channel " << channel << endl;
            channels.insert(channel);
            this->num_threads += 1;
            shared_ptr<Connection>    connection(new Connection(client_addr, *client_port, channel));
            shared_ptr<ServerContext> context(new ServerContext(socket, connection, storage));
            this->create_session(channel, context);
        }
        usleep(10000);
    }
    for (set<int>::iterator channel = channels.begin(); channel != channels.end(); channel++) {
        PLOGI << "Closing socket on channel " << *channel << "..." << endl;
        socket->close(*channel);
        pthread_t thread_id = this->thread_pool.at(*channel);
        PLOGI << "Waiting for thread " << thread_id << " from channel " << *channel << " to teardown..." << endl;
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
    PLOGI << "Connected to " << client_addr << " on channel " << channel << endl;

    if (channel == -1) {
        PLOGE << "Error: Invalid channel: " << channel << endl;
        ::pthread_exit(NULL);
    }
    PLOGD << "Listening on channel " << channel << endl;

    if (session->setup()) {
        PLOGI << "Starting session on channel " << channel << "..." << endl;
        session->run();
        PLOGI << "Session on channel " << channel << " finished." << endl;
    }

    PLOGI << "Closing channel " << channel << "..." << endl;
    session->teardown();
    ::pthread_exit(NULL);
    return NULL;
}

bool Session::setup() {
    PLOGI << "Setting up session..." << endl;
    unique_ptr<uint8_t> buffer((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t)));
    int                 channel      = context->connection->channel;
    int                 payload_size = context->socket->get_message_sync(buffer.get(), channel);
    if (payload_size == 0) {
        PLOGW << "Client " << context->connection->get_full_address() << " has disconnected before setup.." << endl;
        return false;
    }

    unique_ptr<Packet> packet(new Packet(buffer.get()));

    if (packet->type != SessionInit) {
        PLOGW << "Invalid session packet: " << packet->type << endl;
        return false;
    }

    shared_ptr<SessionRequest> request(new SessionRequest(packet->payload));
    // if (!regex_match(string(request->username), regex("[A-Za-z0-9_-]+"))) {
    //     PLOGI << "Invalid username: " << request->username << endl;
    //     return false;
    // }
    shared_ptr<Connection> connection = this->context->connection;
    if (request->type == Unknown) {
        PLOGW << "Invalid session from device " << connection->get_full_address() << " and user " << request->username << endl;
        return false;
    }
    PLOGI << "Established session with user " << request->username << endl;
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
    PLOGI << oss.str();
    shared_ptr<Event> accept_evt(new Event(SessionAccepted, oss.str()));
    accept_evt->send(context->socket, channel);
    return true;
}

Session::Session(shared_ptr<ServerContext> context) {
    this->context = context;
}

void Session::run() {
    PLOGD << "Running session of type " << this->type << endl;
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
        PLOGI << "Invalid session type: " << this->type << endl;
        return;
    }
    }
};

void Session::teardown() {
    int    channel      = context->connection->channel;
    string full_address = context->connection->get_full_address();
    if (context->device != NULL) {
        string username = context->device->username;
        PLOGI << "Unregistering connection from device " << full_address << " from user " << username << endl;
        context->storage->unregister_connection(context);
    } else {
        PLOGI << "Unregistering unauthenticated connection from device " << full_address << endl;
    }
    context->socket->close(channel);
}
