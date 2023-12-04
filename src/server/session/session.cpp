#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstring>
#include <regex>
#include <pthread.h>

#include "session.hpp"
#include "../userland/models.hpp"
#include "../eventhub/publisher.hpp"
#include "../eventhub/subscriber.hpp"
#include "../eventhub/file_sync.hpp"
#include "../../common/eventhub/models.hpp"

using namespace std;



SessionManager::SessionManager(shared_ptr<Socket> socket) {
    this->socket = socket;
    this->num_threads = 0;
}

void SessionManager::start() {
    cout << "Waiting for connections...\n\n";
    shared_ptr<char> client_addr((char *)malloc(sizeof(char) * INET_ADDRSTRLEN));
    shared_ptr<int> client_port((int *)malloc(sizeof(int)));
    shared_ptr<UserStore> storage(new UserStore());
    while(!*this->interrupt) {
        int channel = this->socket->accept(client_addr.get(), client_port.get());
        if (channel != -1) {
            cout << "Connected to " << client_addr.get() << ":" << *client_port.get() << " on channel " << channel << endl;
            this->channels[this->num_threads] = channel;
            this->num_threads += 1;
            shared_ptr<Connection> connection(new Connection(client_addr.get(), *client_port, channel));
            shared_ptr<ServerContext> context(new ServerContext(socket, connection, storage));
            this->create_session(channel, context);
        }
        usleep(10000);
    }
    for(int thread_id = 0; thread_id < num_threads; thread_id++) {
        pthread_join(this->thread_pool[thread_id], NULL);
        int channel = channels[thread_id];
        socket->close(channel);
    }
}

void SessionManager::create_session(int channel, shared_ptr<ServerContext> context) {
    cout << "Spawning new connection on channel " << channel << endl;
    shared_ptr<Connection> connection = context->connection;
    connection->set_thread_id(&thread_pool[num_threads]);
    Session *session = new Session(context);
    pthread_create(&thread_pool[num_threads], NULL, this->handle_session, session);
}


void* SessionManager::handle_session(void *session_ptr) {
    shared_ptr<Session> session((Session *)session_ptr);
    int channel = session->context->connection->channel;

    cout << "Running new thread for channel " << channel << endl;
    string client_addr = session->context->connection->get_full_address();
    cout << "Connected to " << client_addr  << " on channel " << channel << endl;

    if (channel == -1) {
        cout << "Error: Invalid channel: " << channel << endl;
        ::pthread_exit(NULL);
    }
    cout << "Listening on channel " << channel << endl;

    if (session->setup()) {
        session->run();
        // session->loop();
    }

    cout << "Closing channel " << channel << "..." << endl;
    session->teardown();
    ::pthread_exit(NULL);
    return NULL;
}


bool Session::setup() {
    shared_ptr<uint8_t> buffer((uint8_t *)malloc(sizeof(uint8_t) * context->socket->buffer_size));
    int channel = context->connection->channel;
    int payload_size = context->socket->get_message_sync(buffer.get(), channel);
    if (payload_size == 0) {
        return false;
    }

    unique_ptr<Packet> packet(new Packet(buffer.get()));

    if (packet->type != SessionInit) {
         cout << "Invalid session packet: " << packet->type << endl;
        return false;
    }

    shared_ptr<SessionRequest> request(new SessionRequest(packet->payload));
    if (!regex_match(string(request->username), regex("[A-Za-z0-9_-]+"))) {
        cout << "Invalid username: " << request->username << endl;
        return false;
    }
    shared_ptr<Connection> connection = this->context->connection;
    if (request->type == Unknown) {
        cout << "Invalid session from device " << connection->get_full_address() << " and user " << request->username << endl;
        return false;
    }
    cout << "Established session with user " << request->username << endl;
    shared_ptr<UserStore> storage = this->context->storage;
    storage->add_user(request->username);
    if (!storage->register_connection(request->username, context)) {
        cout << "Cannot establish connection for user " << request->username
            << " on device " << connection->get_full_address() << endl;
        return false;
    }
    this->type = request->type;
    string session_type = session_type_map.at(request->type);
    ostringstream oss;
    oss << "Session of type " << session_type << " established." << endl;
    shared_ptr<Event> accept_evt(new Event(SessionAccepted, oss.str()));
    unique_ptr<Packet> evt_packet(new Packet(accept_evt));
    evt_packet->send(context->socket, channel);
    return true;
}


Session::Session(shared_ptr<ServerContext> context) {
    this->context = context;
    this->buffer = new uint8_t[context->socket->buffer_size];
}


void Session::run() {
   switch(type) {
       case FileExchange: {
            FileSync *file_sync = new FileSync(this->context);
            file_sync->loop();
           break;
        }
        case CommandPublisher: {
            ServerEventPublisher *publisher = new ServerEventPublisher(this->context);
            publisher->loop();
            break;
        }
        case CommandSubscriber: {
            ServerEventSubscriber *subscriber = new ServerEventSubscriber(this->context);
            subscriber->loop();
            break;
        }
        default: {
            cout << "Invalid session type: " << this->type << endl;
            return;
        }
   }
};


int Session::loop() {
    int channel = context->connection->channel;
    int payload_size = context->socket->get_message_sync(this->buffer, channel);
    while (payload_size != -1 && payload_size != 0) {
        uint8_t * buf = (uint8_t *)this->buffer;
        Packet *packet = new Packet(buf);
        switch(packet->type) {
            case CommandMsg: {
                cout << "Received message on channel " << channel << ": " << packet->payload
                    << "with size " << packet->total_size << endl;
                cout << "User still is " << context->device->username << endl;
                break;
            }
            default: {
                break;
            }
        }
        payload_size = context->socket->get_message_sync(this->buffer, channel);
    }
    return 0;
}

void Session::teardown() {
    int channel = context->connection->channel;
    string full_address = context->connection->get_full_address();
    // cout << "Unregistering connection from device " << full_address << " from user " << context->connection->device->username << endl;
    context->storage->unregister_connection(context);
    context->socket->close(channel);
}
