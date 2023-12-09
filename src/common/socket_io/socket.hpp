#ifndef SOCKET_H
#define SOCKET_H

#include<sys/socket.h>
#include<netinet/in.h>
#include <iostream>
#include <memory>

using namespace std;

enum SocketMode {
    Server, Client
};

class Socket {
    struct sockaddr_in server_address;
    SocketMode mode;
    // socklen_t addr_size;
    void init(string address, int port, bool *interrupt, SocketMode mode, int buffer_size, int max_requests);

    public:
        int buffer_size;
        int socket_flags;
        int socket_fd;
        bool *interrupt;

        Socket(string address, int port, bool *interrupt, SocketMode mode, int buffer_size);
        Socket(string address, int port, bool *interrupt, SocketMode mode, int buffer_size, int max_requests);
        int listen(int max_requests);
        void connect(string address, int port);
        void bind();
        int accept(char *client_addr, int *client_port);
        int send(uint8_t* bytes, size_t size, int channel);
        int receive(uint8_t *buffer, int channel);
        int close(int channel);
        bool has_error(int channel);
        bool is_connected(int channel);
        // bool get_client_info(int channel, char **client_addr, int *client_port);
        bool has_event(int channel);
        int get_message_sync(uint8_t *buffer, int channel);
};

enum PacketType {
    SessionInit, FileChunk, FileMetadataMsg, ErrorMsg, EventMsg, CommandMsg, 
};

class Packet {

    public:
        PacketType type;
        uint16_t seq_index;
        uint32_t total_size;
        uint16_t payload_size;
        uint8_t *payload;

        Packet(uint8_t *bytes);
        Packet(PacketType type, uint16_t seq_index, uint32_t total_size, uint16_t payload_size, uint8_t *payload);
        ~Packet();
        static int get_max_payload_size();
        size_t to_bytes(uint8_t** bytes_ptr);
        int send(shared_ptr<Socket> socket, int channel);
};


#endif
