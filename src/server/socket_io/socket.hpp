#ifndef SOCKET_H
#define SOCKET_H

#include<sys/socket.h>
#include<netinet/in.h>

class Socket {
    int server_socket;
    struct sockaddr_in server_address;
    struct sockaddr_storage server_storage;
    socklen_t addr_size;

    public:
        int buffer_size;
        int socket_flags;

        Socket(char *address, int port, int buffer_size, int max_requests);
        int listen(int max_requests);
        void bind();
        int accept();
        int send(char *payload, int channel);
        int receive(char *buffer, int channel);
        int close(int channel);
        bool has_error(int channel);
        bool is_connected(int channel);
        bool get_client_info(int channel, char *client_addr, int *addr_len);
        bool has_event(int channel);
};
#endif
