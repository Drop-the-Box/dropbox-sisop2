#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h> // for open
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <memory>

// Project-level imports
#include "../vars.hpp"
#include "socket.hpp"
#include <iostream>
#include <string>

using namespace std;


Socket::Socket(string address, int port, SocketMode mode, int buffer_size) {
    this->init(address, port, mode, buffer_size, 0);
};


Socket::Socket(string address, int port, SocketMode mode, int buffer_size, int max_requests) {
    this->init(address, port, mode, buffer_size, max_requests);
};

void Socket::init(
    string address, int port, SocketMode mode = Server, int buffer_size = BUFFER_SIZE, int max_requests = MAX_REQUESTS
) {

    if (mode == Server) {
        this->socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    } else {
        this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (this->socket_fd == -1) {
        cerr << "Error: cannot open socket on " << address << ":" << port << endl;
    }

    // Set flags for non-blocking
    int flags = fcntl(this->socket_fd, F_GETFL);
    flags = flags == -1 ? O_NONBLOCK : flags | O_NONBLOCK;
    ::fcntl(this->socket_fd, F_SETFL, flags);

    this->socket_flags = flags;
    this->server_address.sin_family = AF_INET;
    this->server_address.sin_port = htons(port);
    this->server_address.sin_addr.s_addr = inet_addr(address.c_str());
    this->buffer_size = buffer_size;

    memset(this->server_address.sin_zero, '\0', sizeof this->server_address.sin_zero);

    this->mode = mode;
    if (mode == Server) {
        this->bind();
        this->listen(max_requests);
    } else {
        shared_ptr<struct hostent> server_addr(gethostbyname(address.c_str()));
        if (server_addr == NULL) {
            cerr << "Error: cannot find address " << address << endl;
        }
        this->server_address.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)server_addr->h_addr));
        this->connect(address, port);
    }
};

void Socket::connect(string address, int port) {
    if (::connect(this->socket_fd ,(sockaddr *)&server_address, sizeof(server_address)) < 0 && errno != EINPROGRESS) {
        cout << "Error: cannot connect to socket on " << address.c_str() << ":" << port <<" detail: " << strerror(errno) << endl;
    }

};


void Socket::bind() {
    struct sockaddr * server_address = (struct sockaddr *) &this->server_address;
    int bind_result = ::bind(this->socket_fd, server_address, sizeof this->server_address);
    if (bind_result != 0) {
        std::cout << "Error binding socket: " << errno << std::endl;
        throw;
    }
}


int Socket::listen(int max_requests) {
    return ::listen(this->socket_fd, max_requests);
};


int Socket::accept(char* client_addr, int *client_port) {
    shared_ptr<struct sockaddr_in> client((struct sockaddr_in *)malloc(sizeof(struct sockaddr_in)));
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    int is_accepted = ::accept(this->socket_fd, (sockaddr *)client.get(), &client_addr_len);
    if (is_accepted != -1) {
        inet_ntop(AF_INET, &client->sin_addr.s_addr, client_addr, INET_ADDRSTRLEN);
        *client_port = htons(client->sin_port);
    }
    return is_accepted;
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


int Socket::receive(uint8_t *buffer, int channel) {
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
    std::cout << "Error? " << status << std::endl;
    return status != 0;
}


bool Socket::is_connected(int channel) {
    return get_client_info(channel, NULL, NULL);
}


bool Socket::get_client_info(int channel, char **client_addr, int *client_port) {
    struct sockaddr_in client;
    socklen_t client_len;
    int is_connected = getpeername(channel, (struct sockaddr *)&client, &client_len);
    char* addr = inet_ntoa(client.sin_addr);
    int port = ntohs(client.sin_port);
    if (client_addr != NULL) {
        *client_port = port;
        memcpy(*client_addr, addr, client_len);
    }
    // std::cout <<  "Conn info: " << addr << ":" << port << is_connected << " status: "<<  is_connected << std::endl;
    return is_connected == 0;
}


int Socket::send(uint8_t *bytes, size_t size, int channel) {
    unique_ptr<uint8_t> buffer((uint8_t*)malloc(size * sizeof(uint8_t))); 
    copy(bytes, bytes+size, buffer.get());
    std::cout << "Sending message: " << buffer.get() << std::endl;
    return ::send(channel, buffer.get(), size * sizeof(uint8_t), 0);
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


int Socket::get_message_async(uint8_t *buffer, int channel) {
    ::memset(buffer, 0, this->buffer_size);
    int payload_size = this->receive(buffer, channel);
    if (payload_size == 0) {
        std::cout << "Client on channel " << channel << " has been disconnected." << std::endl;
    }
    return payload_size;
}

int Socket::get_message_sync(uint8_t *buffer, int channel) {
    ::memset(buffer, 0, this->buffer_size);
    int payload_size = this->get_message_async(buffer, channel);
    while(this->is_connected(channel) && !this->has_error(channel) && payload_size == -1) {
        payload_size = this->get_message_async(buffer, channel);
    }
    if (!this->is_connected(channel) && this->has_error(channel) && payload_size == 0) {
        return 0;
    }
    return payload_size;
}

Packet::Packet(uint8_t *bytes) {
    uint8_t *cursor = bytes;
    size_t uint16_s = sizeof(uint16_t);
    uint16_t prop;

    memcpy(&prop, cursor, uint16_s);
    this->type = (PacketType)ntohs(prop);
    cursor += uint16_s;

    memcpy(&prop, cursor, uint16_s);
    this->seq_index = ntohs(prop);
    cursor += uint16_s;

    uint32_t total_size;
    memcpy(&total_size, cursor, sizeof(uint32_t));
    this->total_size = ntohl(total_size);
    cursor += sizeof(uint32_t);

    memcpy(&prop, cursor, uint16_s);
    this->payload_size = ntohs(prop);
    cursor += uint16_s;

    this->payload = (uint8_t *)malloc(this->payload_size);
    memcpy(this->payload, cursor, this->payload_size);
};


Packet::Packet(
    PacketType type, uint16_t seq_index, uint32_t total_size,
    uint16_t payload_size, uint8_t *payload
) {
    this->type = type;
    this->seq_index = seq_index;
    this->total_size = total_size;
    this->payload_size = payload_size;
    this->payload = (uint8_t *)malloc(payload_size * sizeof(uint8_t));
    memcpy(this->payload, payload, payload_size * sizeof(uint8_t));
};
        
Packet::Packet(shared_ptr<Event> event) {
    this->type = EventMsg;
    this->seq_index = 1;
    this->total_size = event->to_bytes(&this->payload);
    this->payload_size = total_size;
};

size_t Packet::to_bytes(uint8_t** bytes_ptr) {
    size_t packet_size = sizeof(this->type);
    packet_size += sizeof(this->seq_index);
    packet_size += sizeof(uint32_t);
    packet_size += sizeof(this->payload_size);
    packet_size += this->payload_size * sizeof(uint8_t);
    *bytes_ptr = (uint8_t *)malloc(packet_size);

    uint8_t *cursor = *bytes_ptr;
    size_t uint16_s = sizeof(uint16_t);
    uint16_t prop = htons(this->type);
    memcpy(cursor, &prop, uint16_s);
    cursor += uint16_s;

    prop = htons(this->seq_index);
    memcpy(cursor, &prop, uint16_s);
    cursor += uint16_s;

    uint32_t total_size = htonl(this->total_size);
    memcpy(cursor, &total_size, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    prop = htons(this->payload_size);
    memcpy(cursor, &prop, uint16_s);
    cursor += uint16_s;

    memcpy(cursor, this->payload, this->payload_size * sizeof(uint8_t));
    return packet_size;
}


int Packet::send(shared_ptr<Socket> socket, int channel) {
    uint8_t *bytes;
    size_t packet_size = this->to_bytes(&bytes);
    int bytes_sent = socket->send(bytes, packet_size, channel);
    free(bytes);
    if (bytes_sent < 0) {
        cerr << "Cannot write to socket. Reason: " << strerror(errno) << endl;
    }
    cout << "Sent packet with " << bytes_sent << " bytes" << endl;
    return bytes_sent;
}
