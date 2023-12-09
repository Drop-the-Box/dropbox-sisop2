#include "models.hpp"
#include <memory>
#include <cstdlib>
#include <cstring>
#include <plog/Log.h>

#include "../vars.hpp"
#include "../socket_io/socket.hpp"


SessionRequest::SessionRequest(uint8_t *bytes) {
    uint8_t *cursor = bytes;
    size_t uint16_s = sizeof(uint16_t);

    uint16_t prop;
    memmove(&prop, cursor, uint16_s);
    this->type = (SessionType)ntohs(prop);
    cursor += uint16_s;

    memmove(&prop, cursor, uint16_s);
    this->uname_s = ntohs(prop);
    cursor += uint16_s;

    int max_uname_s = Packet::get_max_payload_size() - 2 * uint16_s;
    if (uname_s > max_uname_s) {
        PLOGE << "Buffer overflow: " << uname_s - BUFFER_SIZE << " bigger than the buffer" << endl;
        this->uname_s = max_uname_s;
    }
    
    unique_ptr<char> username((char *)calloc(this->uname_s, sizeof(char)));
    memmove(username.get(), cursor, this->uname_s);
    this->username = string(username.get());
    PLOGI << "Username " << this->username << " size: "<< this->uname_s << endl;
    if (
            this->type != CommandSubscriber && this->type != CommandPublisher
            && this->type != FileExchange
    ) {
        this->type = Unknown;
    }
}


SessionRequest::SessionRequest(SessionType type, string username) {
    this->type = type;
    this->uname_s = username.size() + 1;
    this->username = username;
}


size_t SessionRequest::to_bytes(uint8_t** bytes_ptr) {
    size_t packet_size = sizeof(this->type);
    packet_size += sizeof(this->uname_s);
    packet_size += this->uname_s * sizeof(char);
    *bytes_ptr = (uint8_t *)calloc(packet_size, sizeof(uint8_t));

    uint8_t *cursor = *bytes_ptr;
    size_t uint16_s = sizeof(uint16_t);
    uint16_t prop = htons(this->type);
    memmove(cursor, &prop, uint16_s);
    cursor += uint16_s;

    prop = htons(this->uname_s);
    memmove(cursor, &prop, uint16_s);
    cursor += uint16_s;

    const char *username_ptr = username.c_str();
    memmove(cursor, username_ptr, this->uname_s);
    // memmove(cursor, this->username, this->uname_s * sizeof(char));
    return packet_size;
};
