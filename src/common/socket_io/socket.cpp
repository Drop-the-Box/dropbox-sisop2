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

Socket::~Socket() {
    this->close(this->socket_fd);
}

void Socket::init(
    string address, int port, bool *interrupt, SocketMode mode = Server, int buffer_size = BUFFER_SIZE, int max_requests = MAX_REQUESTS) {
    this->interrupt = interrupt;
    int res;
    do {
        if (mode == Server) {
            this->socket_fd = socket(PF_INET, SOCK_STREAM, 0);
        } else {
            this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        }
        if (this->socket_fd == -1) {
            PLOGE << "Error: cannot open socket on " << address << ":" << port << endl;
            throw SocketError(strerror(errno));
        }
        res = fcntl(this->socket_fd, F_GETFL);
    }
    while(!(res & O_RDWR));

    struct timeval timeout;
    timeout.tv_sec  = SOCKET_TIMEOUT;
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

    if (setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR, &keepalive, sizeof(int))) {
        PLOGE << "ERR reuseaddr" << endl;
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
            throw SocketError("Cannot find address");
        }
        struct in_addr server_host           = *(in_addr *)server_addr->h_addr;
        this->server_address.sin_addr.s_addr = inet_addr(inet_ntoa(server_host));
        this->connect(address, port);
    }
};

void Socket::connect(string address, int port) {
    int succeeded = ::connect(this->socket_fd, (sockaddr *)&server_address, sizeof(server_address));
    if (succeeded < 0 && errno != EINPROGRESS) {
        PLOGE << "Error: cannot connect to socket on " << address.c_str() << ":" << port << " detail: " << strerror(errno) << endl;
        throw SocketConnectError("Cannot connect to socket");
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
    if (*this->interrupt) {
        throw UserInterruptError("No more connections");
    }
    socklen_t           client_addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in *client          = (struct sockaddr_in *)calloc(1, client_addr_len);
    int                 accepted_fd     = ::accept(this->socket_fd, (sockaddr *)client, &client_addr_len);
    if (accepted_fd == -1 || errno == EAGAIN || errno == EWOULDBLOCK) {
        PLOGD << "Socket timeout on accept " << ::strerror(errno) << std::endl;
    } else {
        inet_ntop(AF_INET, &client->sin_addr.s_addr, client_addr, INET_ADDRSTRLEN);
        *client_port = htons(client->sin_port);
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
        PLOGD << "Error getting event from channel " << channel << ": " << event << std::endl;
        return false;
    } else if (event == 0) {
        return false;
    }
    return FD_ISSET(channel, &channel_list);
}

int Socket::receive(uint8_t *buffer, int channel) {
    // if (!this->has_event(channel)) return -1;
    PLOGD << "Receiving data on channel " << channel << "..." << endl;
    int bytes_received = 0;
    int total_bytes    = 0;
    while (total_bytes < BUFFER_SIZE && !this->has_error(channel)) {
        PLOGD << "total_bytes: " << total_bytes << endl;
        bytes_received = ::recv(channel, buffer + total_bytes, BUFFER_SIZE - total_bytes, 0);
        // PLOGE << "bytes_received: " << bytes_received << endl;
        if (bytes_received > 0) {
            PLOGD << "Received data on channel " << channel << ": " << bytes_received << " bytes"
                << "\n\n";
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            PLOGD << "Socket error on channel " << channel << ": " << ::strerror(errno) << std::endl;
            throw SocketTimeoutError(strerror(errno));
        } else if (errno) {
            PLOGD << "Unrecoverable socket error on channel " << channel << ": " << ::strerror(errno) << std::endl;
            throw SocketError(strerror(errno));
        }
        if (bytes_received == 0) {
            PLOGD << "Bytes received is 0." << endl;
            throw SocketDisconnectedError("Server has disconnected.");
        }
        total_bytes += bytes_received;
    }
    PLOGD << "Received " << total_bytes << " bytes on channel " << channel << std::endl;
    return total_bytes;
}

bool Socket::has_error(int channel) {
    if (*this->interrupt) {
        PLOGW << "Service interrupted by command line!" << endl;
        throw UserInterruptError("Service interrupted by user.");
    }
    int error = 0;
    socklen_t err_len = sizeof(error);
    int status  = ::getsockopt(channel, SOL_SOCKET, SO_ERROR, &error, &err_len);
    if (status != 0) {
        string readable_error = errno == 0 ? strerror(status) : strerror(errno);
        PLOGE << "Error getting socket status for channel " << channel << " : " << readable_error << endl;
        throw SocketError(readable_error);
    }
    if (error != 0) {
        string readable_error = strerror(errno);
        PLOGE << "Error getting socket status for channel " << channel << " : " << readable_error <<  "\n\n";
        throw SocketError(readable_error);
    }
    // if(recv(channel,NULL,1, MSG_PEEK | MSG_DONTWAIT) != 0) {
    //     throw SocketError("Socket has been disconnected");
    // }
    return false;
}

bool Socket::is_connected(int channel) {
    return true;
}

int Socket::send(uint8_t *bytes, size_t size, int channel, bool raise_on_timeout) {
    PLOGD << "Sending data on channel " << channel << "..." << std::endl;
    if (this->has_error(channel)) {
        PLOGE << "Channel " << channel << " has error." << std::endl;
        return 0;
    }

    int result = -1;
    if (this->is_connected(channel)) {
        PLOGD << "Channel " << channel << " is connected." << std::endl;
        result = ::send(channel, bytes, BUFFER_SIZE, MSG_NOSIGNAL);
        if (result != -1) {
            PLOGD << "Channel " << channel << " sent " << result << " bytes." << std::endl;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            PLOGE << "Socket timeout on " << channel << ": " << ::strerror(errno) << std::endl;
            throw SocketTimeoutError("Timeout!");
        } else if (errno) {
            PLOGE << "Unrecoverable socket error on channel " << channel << ": " << ::strerror(errno) << endl;
            stringstream oss;
            oss << "Unrecoverable error" << ::strerror(errno);
            throw SocketError(::strerror(errno));
        }
    }
    return result;
};

int Socket::close(int channel) {
    if (!this->is_connected(channel)) {
        PLOGW << "Channel " << channel << " already disconnected." << std::endl;
        return 0;
    }
    PLOGD << "Closing connection on channel " << channel << std::endl;
    if (this->mode == Server) {
        ::shutdown(channel, SHUT_RDWR);
        char buf[1];
        while(::recv(channel, buf, 1, 0) > 0) {};
    }
    int result = ::close(channel);
    if (result == -1) {
        PLOGE << "Error closing channel " << channel << ": " << strerror(errno) << std::endl;
    }
    return result;
}

int Socket::get_message_sync(uint8_t *buffer, int channel, bool raise_on_timeout) {
    if (this->has_error(channel))
        return 0;
    ::memset(buffer, 0, BUFFER_SIZE);
    int payload_size = 0;
    int time_elapsed = 0;
    // while (!payload_size && ioctl(channel, FIONREAD, &payload_size) >= 0) {
    //     PLOGI << "Waiting for data to arrive..." << endl;
    //     if (this->has_error(channel))
    //         throw SocketError(strerror(errno));
    //     if (time_elapsed > SOCKET_TIMEOUT && raise_on_timeout) {
    //         throw SocketTimeoutError("Timeout waiting for data");
    //     }
    //     usleep(1000);
    //     time_elapsed += 0.001;
    // }
    payload_size = this->receive(buffer, channel);
    if (payload_size < 0) {
        ostringstream error;
        error << "Error while reading socket: " << strerror(errno) << endl;
        throw SocketError(error.str());
    }

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

    *bytes_ptr      = (uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t));
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
    int      bytes_sent  = socket->send(bytes, packet_size, channel, true);
    free(bytes);
    if (bytes_sent < 0) {
        PLOGE << "Cannot write to socket. Reason: " << strerror(errno) << endl;
    }
    PLOGD << "Sent packet with " << packet_size << " bytes" << endl;
    return bytes_sent;
}
