#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstring>
#include <regex>
#include <memory>

#include "models.hpp"


Connection::Connection(char *address, int port, int channel) {
    this->address = (char *)malloc(strlen(address) * sizeof(char));
    strcpy(this->address, address);
    this->port = port;
    this->channel = channel;
}

Connection::~Connection(void) {
    free(this->address);
}

void Connection::set_device(Device *device) {
    this->device = device;
}

void Connection::set_thread_id(pthread_t *thread_id) {
    this->thread_id = thread_id;
}

void Connection::set_session_type(SessionType session_type) {
    this->session_type = session_type;
}

string Connection::get_full_address() {
    ostringstream oss;
    oss << this->address << ":" << this->port;
    std::string conn_info = oss.str();
    return conn_info;
}

SessionRequest::SessionRequest(uint8_t *bytes) {
    uint8_t *cursor = bytes;
    size_t uint16_s = sizeof(uint16_t);
    uint16_t prop;
    memcpy(&prop, cursor, uint16_s);
    this->type = (SessionType)ntohs(prop);
    cursor += uint16_s;
    memcpy(&prop, cursor, uint16_s);
    this->uname_s = ntohs(prop);
    cursor += uint16_s;
    this->username = (char *)malloc(this->uname_s);
    memcpy(this->username, cursor, this->uname_s);
    if (this->type != CommandSubscriber && this->type != CommandPublisher && this->type != FileExchange) {
        this->type = Unknown;
    }
}

SessionRequest::~SessionRequest(void) {
    free(this->username);
}

SessionRequest::SessionRequest(SessionType type, char *username) {
    this->type = type;
    this->uname_s = strlen(username);
    this->username = (char *)malloc(this->uname_s * sizeof(char));
    strcpy(this->username, username);
}

size_t SessionRequest::to_bytes(uint8_t** bytes_ptr) {
    size_t packet_size = sizeof(this->type);
    packet_size += sizeof(this->uname_s);
    packet_size += this->uname_s * sizeof(char);
    *bytes_ptr = (uint8_t *)malloc(packet_size);

    uint8_t *cursor = *bytes_ptr;
    size_t uint16_s = sizeof(uint16_t);
    uint16_t prop = htons(this->type);
    memcpy(cursor, &prop, uint16_s);
    cursor += uint16_s;
    prop = htons(this->uname_s);
    memcpy(cursor, &prop, uint16_s);
    cursor += uint16_s;
    memcpy(cursor, this->username, this->uname_s * sizeof(char));
    return packet_size;
};
