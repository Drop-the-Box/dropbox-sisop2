#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h> // for open
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>

// Project-level imports
#include "socket.hpp"


Socket::Socket(char *address, int port, int buffer_size = 1024, int max_requests = 10) {
    this->server_socket = socket(PF_INET, SOCK_STREAM, 0);

    // Set flags for non-blocking
    int flags = fcntl(this->server_socket, F_GETFL);
    flags = flags == -1 ? O_NONBLOCK : flags | O_NONBLOCK;
    ::fcntl(this->server_socket, F_SETFL, flags);

    this->socket_flags = flags;
    this->server_address.sin_family = AF_INET;
    this->server_address.sin_port = htons(port);
    this->server_address.sin_addr.s_addr = inet_addr(address);
    this->addr_size = strlen(address);
    this->buffer_size = buffer_size;

    memset(this->server_address.sin_zero, '\0', sizeof this->server_address.sin_zero);

    this->bind();
    this->listen(max_requests);
};


void Socket::bind() {
    struct sockaddr * server_address = (struct sockaddr *) &this->server_address;
    int bind_result = ::bind(this->server_socket, server_address, sizeof this->server_address);
    if (bind_result != 0) {
        std::cout << "Error binding socket: " << errno << std::endl;
        throw;
    }
}


int Socket::listen(int max_requests) {
    return ::listen(this->server_socket, max_requests);
};


int Socket::accept() {
    struct sockaddr* server_storage = (struct sockaddr *) &this->server_storage;
    return ::accept(this->server_socket, server_storage, &this->addr_size);
};


bool Socket::has_event(int channel) {
    fd_set channel_list;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;
    FD_ZERO(&channel_list);
    FD_SET(channel, &channel_list);
    int event = select(channel + 1, &channel_list, NULL, NULL, &timeout);
    if (event < 0) {
        std::cout << "Error getting event from channel " <<  channel << ": " << event << std::endl;
        return false;
    } else if (event == 0) {
        // std::cout << "Timeout occurred! No data after 100 miliseconds.\n";
        return false;
    }
    return FD_ISSET(channel, &channel_list);
}


int Socket::receive(char *buffer, int channel) {
    if (!this->has_event(channel)) return -1;
    // std::cout << "Receiving data on channel " << channel << "..." << std::endl;
    int error_code = 0;
    int result = ::recv(channel, buffer, this->buffer_size, 0);
    if (result <= 0) {
        return result;
    }
    if (error_code == EAGAIN || error_code == EWOULDBLOCK) {
        std::cout << "Socket error on channel " << channel << ": " << ::strerror(error_code) << std::endl;
        return 0;
    } else if (error_code) {
        std::cout << "Unrecoverable socket error on channel " << channel << ": " << ::strerror(error_code) << std::endl;
        return 0;
    }
    // std::cout << "Received data on channel " << channel << ": " << buffer << "\n\n";
    return result;
}


bool Socket::has_error(int channel) {
    return false;  // to be debugged
    int error;
    socklen_t err_len = sizeof (error);
    int status = ::getsockopt(channel, SOL_SOCKET, SO_ERROR, &error, &err_len);
    if (status != 0) {
        std::cout << "Error getting socket status: " << strerror(error) << "\n\n";
    }
    return status != 0;
}


bool Socket::is_connected(int channel) {
    int is_connected = get_client_info(channel, NULL, NULL);
    if (is_connected != 0) {
        // std::cout << "Channel " << channel << " not connected." << std::endl;
    }
    return is_connected == 0;
}


bool Socket::get_client_info(int channel, char *client_addr, int *addr_len) {
    sockaddr *client = (sockaddr *)calloc(sizeof(sockaddr_in), 0);
    socklen_t *client_len = (socklen_t *)calloc(sizeof(int), 0);
    if (client_addr != NULL) {
        client = (sockaddr *)client_addr;
        client_len = (socklen_t *)addr_len;
    }
    int is_connected = getpeername(channel, client, client_len);
    if (client_addr == NULL) {
        free(client);
        free(client_len);
    }
    return is_connected;
}


int Socket::send(char *message, int channel) {
    char *buffer =  (char*)(malloc(sizeof(this->buffer_size))); 
    std::cout << "Sending message: " << buffer << std::endl;
    strcpy(buffer, message);
    free(buffer);
    ::send(channel, message, sizeof(message), 0);
    return 0;
};


int Socket::close(int channel) {
    if (!this->is_connected(channel)) {
        std::cout << "Channel " << channel << " already disconnected." << std::endl;
        return 0;
    }
    std::cout << "Closing connection on channel " << channel << std::endl;
    int result = ::close(channel);
    if (result == -1) {
        std::cout << "Error closing channel " << channel << ": " << strerror(errno) << std::endl;
    }
    return result;
}
