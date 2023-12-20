#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <iostream>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <plog/Log.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

// Project-level imports
#include "../vars.hpp"
#include "socket.hpp"
#include <iostream>
#include <string>

using namespace std;

Socket::Socket(string address, int port, bool *interrupt, SocketMode mode, int buffer_size) {
    this->init(address, port, interrupt, mode, buffer_size, 0);
};

Socket::Socket(string address, int port, bool *interrupt, SocketMode mode, int buffer_size, int max_requests) {
    this->init(address, port, interrupt, mode, buffer_size, max_requests);
};

void Socket::init(
    string address, int port, bool *interrupt, SocketMode mode = Server, int buffer_size = BUFFER_SIZE, int max_requests = MAX_REQUESTS) {
    this->interrupt = interrupt;
    if (mode == Server) {
        this->socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    } else {
        this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (this->socket_fd == -1) {
        PLOGE << "Error: cannot open socket on " << address << ":" << port << endl;
    }

    // Options for non-blocking socket that didn't work
    // Set flags for non-blocking
    // int flags = fcntl(this->socket_fd, F_GETFL);
    // flags = flags == -1 ? O_NONBLOCK : flags | O_NONBLOCK;
    // ::fcntl(this->socket_fd, F_SETFL, flags);
    // this->socket_flags = flags;

    struct timeval timeout;
    timeout.tv_sec  = 10;
    timeout.tv_usec = 0;
    int keepalive = 1;

    if (setsockopt(this->socket_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof (int))) {
        PLOGE << "Cannot set socket as keepalive. Reason: " << strerror(errno) << endl;
    }

    if (setsockopt(this->socket_fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof (int))) {
        PLOGE << "Cannot set socket recv buffer size. Reason: " << strerror(errno) << endl;
    }

    if (setsockopt(this->socket_fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof (int))) {
        PLOGE << "Cannot set socket send buffer size. Reason: " << strerror(errno) << endl;
    }

    if (setsockopt(this->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0) {
        PLOGE << "Cannot set socket recv timeout. Reason: " << strerror(errno) << endl;
    }

    if (setsockopt(this->socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout) < 0) {
        PLOGE << "Cannot set socket send timeout. Reason: " << strerror(errno) << endl;
    }

    this->server_address.sin_family      = AF_INET;
    this->server_address.sin_port        = htons(port);
    this->server_address.sin_addr.s_addr = inet_addr(address.c_str());
    this->buffer_size                    = buffer_size;

    memset(this->server_address.sin_zero, '\0', sizeof this->server_address.sin_zero);

    this->mode = mode;
    if (mode == Server) {
        this->bind();
        this->listen(max_requests);
    } else {
        struct hostent *server_addr = gethostbyname(address.c_str());
        if (server_addr == NULL) {
            PLOGE << "Error: cannot find address " << address << endl;
        }
        struct in_addr server_host           = *(in_addr *)server_addr->h_addr;
        this->server_address.sin_addr.s_addr = inet_addr(inet_ntoa(server_host));
        this->connect(address, port);
    }
};

void Socket::connect(string address, int port) {
    if (::connect(this->socket_fd, (sockaddr *)&server_address, sizeof(server_address)) < 0 && errno != EINPROGRESS) {
        PLOGE << "Error: cannot connect to socket on " << address.c_str() << ":" << port << " detail: " << strerror(errno) << endl;
    }
};

void Socket::bind() {
    struct sockaddr *server_address = (struct sockaddr *)&this->server_address;
    int              bind_result    = ::bind(this->socket_fd, server_address, sizeof this->server_address);
    if (bind_result != 0) {
        PLOGE << "Error binding socket: " << errno << std::endl;
        throw;
    }
}

int Socket::listen(int max_requests) {
    return ::listen(this->socket_fd, max_requests);
};

int Socket::accept(char *client_addr, int *client_port) {
    socklen_t           client_addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in *client          = (struct sockaddr_in *)calloc(1, client_addr_len);
    int                 accepted_fd     = ::accept(this->socket_fd, (sockaddr *)client, &client_addr_len);
    if (accepted_fd != -1) {
        inet_ntop(AF_INET, &client->sin_addr.s_addr, client_addr, INET_ADDRSTRLEN);
        *client_port = htons(client->sin_port);
        // fcntl(accepted_fd, F_SETFL, this->socket_flags);
    }
    free(client);
    return accepted_fd;
};

bool Socket::has_event(int channel) {
    fd_set         channel_list;
    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = 1000;
    FD_ZERO(&channel_list);
    FD_SET(channel, &channel_list);
    int event = select(channel + 1, &channel_list, NULL, NULL, &timeout);
    if (event < 0) {
        PLOGE << "Error getting event from channel " << channel << ": " << event << std::endl;
        return false;
    } else if (event == 0) {
        // std::cout << "Timeout occurred! No data after 100 miliseconds.\n";
        return false;
    }
    return FD_ISSET(channel, &channel_list);
}

int Socket::receive(uint8_t *buffer, int channel) {
    // if (!this->has_event(channel)) return -1;
    PLOGI << "Receiving data on channel " << channel << "..." << endl;
    int error_code     = 0;
    int bytes_received = 0;
    int total_bytes    = 0;
    while (total_bytes < BUFFER_SIZE) {
        PLOGI << "total_bytes: " << total_bytes << endl;
        bytes_received = ::recv(channel, buffer + total_bytes, BUFFER_SIZE - total_bytes, 0);
        PLOGE << "bytes_received: " << bytes_received << endl;
        if (bytes_received == -1) {
            sleep(3);
            continue;
        }
        PLOGD << "Received data on channel " << channel << ": " << bytes_received << " bytes"
              << "\n\n";
        if (error_code == EAGAIN || error_code == EWOULDBLOCK) {
            PLOGE << "Socket error on channel " << channel << ": " << ::strerror(error_code) << std::endl;
        } else if (error_code) {
            PLOGE << "Unrecoverable socket error on channel " << channel << ": " << ::strerror(error_code) << std::endl;
        }
        if (bytes_received == 0) {
            PLOGW << "Bytes received is 0." << endl;
            return bytes_received;
        }
        total_bytes += bytes_received;
    }
    PLOGI << "Received " << total_bytes << " bytes on channel " << channel << std::endl;
    return total_bytes;
}

bool Socket::has_error(int channel) {
    // return false;  // to be debugged
    if (*this->interrupt) {
        PLOGW << "Service interrupted by command line!" << endl;
        return true;
    }
    int       error;
    socklen_t err_len = sizeof(error);
    int       status  = ::getsockopt(channel, SOL_SOCKET, SO_ERROR, &error, &err_len);
    if (status != 0) {
        PLOGE << "Error getting socket status: " << strerror(error) << "\n\n";
    }
    return status != 0;
}

bool Socket::is_connected(int channel) {
    return true;
}

// bool Socket::get_client_info(int channel, char **client_addr, int *client_port) {
//     unique_ptr<struct sockaddr_in> client((struct sockaddr_in *)calloc(1, sizeof(struct sockaddr_in)));
//     socklen_t client_len;
//     int is_connected = getpeername(channel, (struct sockaddr *)client.get(), &client_len);
//     char* addr = inet_ntoa(client->sin_addr);
//     int port = ntohs(client->sin_port);
//     if (client_addr != NULL) {
//         *client_port = port;
//         memmove(*client_addr, addr, client_len);
//     }
//     return is_connected == 0;
// }

int Socket::send(uint8_t *bytes, size_t size, int channel) {
    int error_code     = 0;
    PLOGI << "Sending data on channel " << channel << "..." << std::endl;
    if (this->has_error(channel)) {
        PLOGI << "Channel " << channel << " has error." << std::endl;
        return 0;
    }

    int result = -1;
    if (this->is_connected(channel)) {
        PLOGI << "Channel " << channel << " is connected." << std::endl;
        result = ::send(channel, bytes, BUFFER_SIZE, 0);
        PLOGI << "Channel " << channel << " sent " << result << " bytes." << std::endl;
        if (error_code == EAGAIN || error_code == EWOULDBLOCK) {
            PLOGE << "Socket error on channel " << channel << ": " << ::strerror(error_code) << std::endl;
        } else if (error_code) {
            PLOGE << "Unrecoverable socket error on channel " << channel << ": " << ::strerror(error_code) << std::endl;
        }
    }
    return result;
};

int Socket::close(int channel) {
    if (!this->is_connected(channel)) {
        PLOGW << "Channel " << channel << " already disconnected." << std::endl;
        return 0;
    }
    PLOGI << "Closing connection on channel " << channel << std::endl;
    int result = ::close(channel);
    if (result == -1) {
        PLOGE << "Error closing channel " << channel << ": " << strerror(errno) << std::endl;
    }
    return result;
}

int Socket::get_message_sync(uint8_t *buffer, int channel) {
    if (this->has_error(channel))
        return 0;
    ::memset(buffer, 0, BUFFER_SIZE);
    int payload_size = 0;
    while (!payload_size && ioctl(channel, FIONREAD, &payload_size) >= 0) {
        // PLOGI << "Waiting for data to arrive..." << endl;
        if (*this->interrupt)
            return 0;
        usleep(1000);
    }
    // while(payload_size == -1){
    PLOGI << "in get_message_sync 4" << endl;
    payload_size = this->receive(buffer, channel);
    // }
    PLOGI << "in get_message_sync 5" << endl;
    return payload_size;
}

int Packet::get_max_payload_size() {
    return BUFFER_SIZE - 3 * sizeof(uint16_t) - sizeof(uint32_t);
}

Packet::Packet(uint8_t *bytes) {
    uint8_t *cursor   = bytes;
    size_t   uint16_s = sizeof(uint16_t);
    uint16_t prop;

    memmove(&prop, cursor, uint16_s);
    this->type = (PacketType)ntohs(prop);
    cursor += uint16_s;

    memmove(&prop, cursor, uint16_s);
    this->seq_index = ntohs(prop);
    cursor += uint16_s;

    uint32_t total_size;
    memmove(&total_size, cursor, sizeof(uint32_t));
    this->total_size = ntohl(total_size);
    cursor += sizeof(uint32_t);

    memmove(&prop, cursor, uint16_s);
    this->payload_size = ntohs(prop);
    cursor += uint16_s;

    if (this->payload_size > this->get_max_payload_size()) {
        PLOGE << "Buffer overflow: " << this->payload_size - BUFFER_SIZE << " bigger than the buffer" << endl;
        this->payload_size = this->get_max_payload_size();
    }
    this->payload = (uint8_t *)calloc(this->payload_size, sizeof(uint8_t));
    memmove(this->payload, cursor, this->payload_size);
};

Packet::Packet(
    PacketType type, uint16_t seq_index, uint32_t total_size,
    uint16_t payload_size, uint8_t *payload) {
    this->type         = type;
    this->seq_index    = seq_index;
    this->total_size   = total_size;
    this->payload_size = payload_size;
    this->payload      = (uint8_t *)calloc(payload_size, sizeof(uint8_t));
    memmove(this->payload, payload, payload_size);
};

Packet::~Packet() {
    free(this->payload);
}

size_t Packet::to_bytes(uint8_t **bytes_ptr) {
    size_t uint16_s = sizeof(uint16_t);

    size_t packet_size = uint16_s;
    packet_size += uint16_s;
    packet_size += sizeof(uint32_t);
    packet_size += uint16_s;
    packet_size += this->payload_size;

    *bytes_ptr      = (uint8_t *)calloc(packet_size, sizeof(uint8_t));
    uint8_t *cursor = *bytes_ptr;

    uint16_t prop = htons(this->type);
    memmove(cursor, &prop, uint16_s);
    cursor += uint16_s;

    prop = htons(this->seq_index);
    memmove(cursor, &prop, uint16_s);
    cursor += uint16_s;

    uint32_t total_size = htonl(this->total_size);
    memmove(cursor, &total_size, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    prop = htons(this->payload_size);
    memmove(cursor, &prop, uint16_s);
    cursor += uint16_s;

    memmove(cursor, this->payload, this->payload_size);
    return packet_size;
}

int Packet::send(shared_ptr<Socket> socket, int channel) {
    uint8_t *bytes;
    size_t   packet_size = this->to_bytes(&bytes);
    int      bytes_sent  = socket->send(bytes, packet_size, channel);
    free(bytes);
    if (bytes_sent < 0) {
        PLOGE << "Cannot write to socket. Reason: " << strerror(errno) << endl;
        throw;
    }
    PLOGD << "Sent packet with " << packet_size << " bytes" << endl;
    return bytes_sent;
}
