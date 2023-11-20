#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstring>

#include "session.hpp"



SessionManager::SessionManager(Socket *socket) {
    this->socket = socket;
    this->num_threads = 0;
}

void SessionManager::start() {
    std::cout << "Waiting for connections...\n\n";
    while(!*this->interrupt) {
        int channel = this->socket->accept();
        if (channel != -1) {
            this->create_session(channel);
            this->channels[this->num_threads] = channel;
            this->num_threads += 1;
        }
    }
    for(int thread_id = 0; thread_id < num_threads; thread_id++) {
        int channel = channels[thread_id];
        socket->close(channel);
    }
    ::pthread_exit(NULL);
}


void SessionManager::create_session(int channel) {
    std::cout << "Spawning new connection on channel " << channel << std::endl;
    Session *session = new Session(socket, channel, &thread_pool[num_threads]);
    pthread_create(&thread_pool[num_threads], NULL, this->handle_session, session);
}


void* SessionManager::handle_session(void *session_ptr) {
    Session *session = (Session *)session_ptr;
    int channel = session->channel;

    std::cout << "Running new thread for channel " << channel << std::endl;
    if (channel == -1) {
        std::cout << "Error: Invalid channel: " << channel << std::endl;
        ::pthread_exit(NULL);
    }
    std::cout << "Listening on channel " << channel << std::endl;

    Socket *socket = session->socket;
    session->stream_messages();

    std::cout << "Closing channel " << channel << "..." << std::endl;
    socket->close(channel);
    free(session);
    return NULL;
}


Session::Session(Socket *socket, int channel, pthread_t *thread) {
    this->socket = socket;
    this->channel = channel;
    this->thread = thread;
    this->buffer = new char[socket->buffer_size];
}


int Session::get_message() {
    ::memset(buffer, 0, socket->buffer_size);
    int payload_size = socket->receive(this->buffer, this->channel);
    if (payload_size == 0) {
        std::cout << "Client on channel " << this->channel << " has been disconnected." << std::endl;
    }
    return payload_size;
}


int Session::stream_messages() {
    int payload_size;
    while(!this->socket->has_error(channel) && this->socket->is_connected(channel)) {
        // std::cout << "No errors in channel " << channel << "\n\n";
        payload_size = this->get_message();
        if (payload_size != -1) {
            std::cout << "Received message on channel " << channel << ": " << this->buffer << std::endl;
        }
        if (payload_size == 0) {
            break;
        }
    }
    return 0;
}
