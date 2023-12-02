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

using namespace std;



SessionManager::SessionManager(shared_ptr<Socket> socket) {
    this->socket = socket;
    this->num_threads = 0;
}

void SessionManager::start() {
    cout << "Waiting for connections...\n\n";
    shared_ptr<char> client_addr((char *)malloc(sizeof(char) * INET_ADDRSTRLEN));
    shared_ptr<int> client_port((int *)malloc(sizeof(int)));
    UserStore *storage = new UserStore();
    while(!*this->interrupt) {
        int channel = this->socket->accept(client_addr.get(), client_port.get());
        if (channel != -1) {
            cout << "Connected to " << client_addr.get() << ":" << *client_port.get() << " on channel " << channel << endl;
            this->channels[this->num_threads] = channel;
            this->num_threads += 1;
            Connection *connection = new Connection(client_addr.get(), *client_port, channel);
            this->create_session(channel, connection, storage);
        }
        usleep(10000);
    }
    for(int thread_id = 0; thread_id < num_threads; thread_id++) {
        pthread_join(this->thread_pool[thread_id], NULL);
        int channel = channels[thread_id];
        socket->close(channel);
    }
}


void SessionManager::create_session(int channel, Connection *connection, UserStore *storage) {
    cout << "Spawning new connection on channel " << channel << endl;
    connection->set_thread_id(&thread_pool[num_threads]);
    Session *session = new Session(socket, channel, connection, storage);
    pthread_create(&thread_pool[num_threads], NULL, this->handle_session, session);
}


void* SessionManager::handle_session(void *session_ptr) {
    shared_ptr<Session> session((Session *)session_ptr);
    int channel = session->channel;

    cout << "Running new thread for channel " << channel << endl;
    cout << "Connected to " << session->connection->get_full_address()  << " on channel " << channel << endl;

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
    shared_ptr<uint8_t> buffer((uint8_t *)malloc(sizeof(uint8_t) * socket->buffer_size));
    int payload_size = this->socket->get_message_sync(buffer.get(), this->channel);
    if (payload_size == 0) {
        return false;
    }

    unique_ptr<Packet> packet(new Packet(buffer.get()));

    if (packet->type != ClientSession) {
         cout << "Invalid packet type: " << packet->type << endl;
        return false;
    }

    shared_ptr<SessionRequest> request(new SessionRequest(packet->payload));
    if (!regex_match(string(request->username), regex("[A-Za-z0-9_-]+"))) {
        cout << "Invalid username: " << request->username << endl;
        return false;
    }
    if (request->type == Unknown) {
        cout << "Invalid session from device " << this->connection->get_full_address() << " and user " << request->username << endl;
        return false;
    }
    cout << "Established session with user " << request->username << endl;
    this->storage->add_user(request->username);
    if (!this->storage->register_connection(request->username, this->connection)) {
        cout << "Cannot establish connection for user " << request->username
            << " on device " << this->connection->get_full_address() << endl;
        return false;
    }

    this->device = this->connection->device;
    this->connection->set_session_type(request->type);
    this->type = request->type;
    return true;
}


Session::Session(shared_ptr<Socket> socket, int channel, Connection *connection, UserStore *storage) {
    this->socket = socket;
    this->connection = connection;
    this->channel = channel;
    this->storage = storage;
    this->buffer = new uint8_t[socket->buffer_size];
}


void Session::run() {
   switch(this->type) {
       case FileExchange: {
           break;
        }
        case CommandPublisher: {
            EventPublisher *publisher = new EventPublisher(this->socket, this->connection, this->storage);
            publisher->loop();
            break;
        }
        case CommandSubscriber: {
            EventSubscriber *subscriber = new EventSubscriber(this->socket, this->connection, this->storage);
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
    int payload_size = this->socket->get_message_sync(this->buffer, this->channel);
    while (payload_size != -1 && payload_size != 0) {
        uint8_t * buf = (uint8_t *)this->buffer;
        Packet *packet = new Packet(buf);
        switch(packet->type) {
            case Command: {
                cout << "Received message on channel " << channel << ": " << packet->payload
                    << "with size " << packet->total_size << endl;
                cout << "User still is " << this->device->username << endl;
                break;
            }
            default: {
                break;
            }
        }
        payload_size = this->socket->get_message_sync(this->buffer, this->channel);
    }
    return 0;
}

void Session::teardown() {
    string full_address = this->connection->get_full_address();
    cout << "Unregistering connection from device " << full_address << " from user " << this->connection->device->username << endl;
    this->storage->unregister_connection(this->connection);
    this->socket->close(channel);
}
