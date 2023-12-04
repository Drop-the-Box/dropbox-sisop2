#include "models.hpp"
#include <memory>
#include <cstdlib>
#include <cstring>


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
    unique_ptr<char> username((char *)malloc(this->uname_s));
    ::copy(cursor, cursor+this->uname_s, username.get());
    this->username = string(username.get());
    // memcpy(username, cursor, this->uname_s);
    if (
            this->type != CommandSubscriber && this->type != CommandPublisher
            && this->type != FileExchange
    ) {
        this->type = Unknown;
    }
}


SessionRequest::SessionRequest(SessionType type, string username) {
    this->type = type;
    this->uname_s = username.length();
    this->username = username;
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
    const char *username_ptr = username.c_str();
    ::copy(username_ptr, username_ptr+this->uname_s, cursor);
    // memcpy(cursor, this->username, this->uname_s * sizeof(char));
    return packet_size;
};
