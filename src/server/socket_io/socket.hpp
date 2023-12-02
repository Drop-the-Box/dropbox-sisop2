#ifndef SOCKET_H
#define SOCKET_H

#include<sys/socket.h>
#include<netinet/in.h>
#include <iostream>

using namespace std;

class Socket {
    int server_socket;
    struct sockaddr_in server_address;
    // socklen_t addr_size;

    public:
        int buffer_size;
        int socket_flags;

        Socket(string address, int port, int buffer_size, int max_requests);
        int listen(int max_requests);
        void bind();
        int accept(char *client_addr, int *client_port);
        int send(uint8_t* bytes, size_t size, int channel);
        int receive(uint8_t *buffer, int channel);
        int close(int channel);
        bool has_error(int channel);
        bool is_connected(int channel);
        bool get_client_info(int channel, char **client_addr, int *client_port);
        bool has_event(int channel);
        int get_message_async(uint8_t *buffer, int channel);
        int get_message_sync(uint8_t *buffer, int channel);
};

enum PacketType {
    Command, ClientSession, FileChunk, FileInfo, Error, Event
};

class Packet {

    public:
        PacketType type;
        uint16_t seq_index;
        uint16_t total_size;
        uint16_t payload_size;
        uint8_t *payload;

        Packet(uint8_t *bytes);
        Packet(PacketType type, uint16_t seq_index, uint16_t total_size, uint16_t payload_size, uint8_t *payload);
        size_t to_bytes(uint8_t** bytes_ptr);

};


#endif
