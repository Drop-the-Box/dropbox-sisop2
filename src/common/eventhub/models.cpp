#include "models.hpp"
#include <cstring>
#include <iostream>
#include <memory>
#include <plog/Log.h>

#include "../vars.hpp"

using namespace std;

Event::Event(uint8_t *bytes) {
    uint8_t *cursor   = bytes;
    size_t   uint16_s = sizeof(uint16_t);

    uint16_t prop;
    memmove(&prop, cursor, uint16_s);
    this->type = (EventType)ntohs(prop);
    cursor += uint16_s;

    memmove(&prop, cursor, uint16_s);
    uint16_t payload_size = ntohs(prop);
    PLOGD << "Loaded event with size: " << payload_size << endl;
    cursor += uint16_s;

    int max_payload_size = Packet::get_max_payload_size() - 2 * uint16_s;
    if (payload_size > max_payload_size) {
        PLOGE << "Buffer overflow: " << payload_size - max_payload_size << " bigger than the buffer" << endl;
        payload_size = max_payload_size;
    }
    unique_ptr<uint8_t> payload((uint8_t *)calloc(payload_size, sizeof(uint8_t)));
    memmove(payload.get(), cursor, payload_size * sizeof(uint8_t));
    this->message = string((char *)payload.get());
}

Event::Event(EventType type, string message) {
    this->type    = type;
    this->message = message;
}

bool Event::send(shared_ptr<Socket> socket, int channel) {
    uint16_t            message_size = this->message.size() + 1;
    unique_ptr<uint8_t> message_buf((uint8_t *)calloc(message_size, sizeof(uint8_t)));
    memmove(message_buf.get(), this->message.c_str(), message_size);
    /// PLOGD << "Encoding msg:  " << message_buf.get() << endl;
    size_t packet_size = sizeof(this->type);
    packet_size += sizeof(uint16_t);
    packet_size += message_size * sizeof(uint8_t);

    unique_ptr<uint8_t> buffer((uint8_t *)calloc(packet_size, sizeof(uint8_t)));

    uint8_t *cursor   = buffer.get();
    size_t   uint16_s = sizeof(uint16_t);

    uint16_t prop = htons(this->type);
    memmove(cursor, &prop, uint16_s);
    cursor += uint16_s;

    prop = htons(message_size);
    memmove(cursor, &prop, uint16_s);
    cursor += uint16_s;

    memmove(cursor, message_buf.get(), message_size * sizeof(uint8_t));
    unique_ptr<Packet> packet(new Packet(EventMsg, 1, packet_size, packet_size, buffer.get()));
    packet->send(socket, channel);
    return true;
};
