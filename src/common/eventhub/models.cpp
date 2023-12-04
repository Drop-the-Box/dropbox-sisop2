#include "models.hpp"
#include <iostream>
#include <cstring>
#include <memory>

using namespace std;

Event::Event(uint8_t *bytes) {
    uint8_t *cursor = bytes;
    size_t uint16_s = sizeof(uint16_t);
    uint16_t prop;
    memcpy(&prop, cursor, uint16_s);
    this->type = (EventType)ntohs(prop);
    cursor += uint16_s;
    memcpy(&prop, cursor, uint16_s);
    uint16_t payload_size = ntohs(prop);
    cout << "Loaded event with size: " << payload_size << endl;
    cursor += uint16_s;
    uint8_t *payload = (uint8_t *)malloc(payload_size * sizeof(uint8_t));
    memcpy(payload, cursor, payload_size * sizeof(uint8_t));
    this->message = string((char *)payload);
    free(payload);
}


Event::Event(EventType type, string message) {
    this->type = type;
    this->message = message;

}


size_t Event::to_bytes(uint8_t** bytes_ptr) {
    uint16_t message_size = this->message.length();
    uint8_t *message_ptr = (uint8_t *)malloc(message_size * sizeof(uint8_t));
    strcpy((char *)message_ptr, this->message.c_str());
    cout << "Encoding msg:  " << message_ptr << endl;
    size_t packet_size = sizeof(this->type);
    packet_size += sizeof(uint16_t);
    packet_size += message_size * sizeof(uint8_t);
    *bytes_ptr = (uint8_t *)malloc(packet_size);

    uint8_t *cursor = *bytes_ptr;
    size_t uint16_s = sizeof(uint16_t);
    uint16_t prop = htons(this->type);
    memcpy(cursor, &prop, uint16_s);
    cursor += uint16_s;
    prop = htons(message_size);
    memcpy(cursor, &prop, uint16_s);
    cursor += uint16_s;
    memcpy(cursor, message_ptr, message_size * sizeof(uint8_t));
    free(message_ptr);
    return packet_size;

}
