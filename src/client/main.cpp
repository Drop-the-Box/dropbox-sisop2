#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sstream>
#include <iostream>
#include <string>
#include <plog/Log.h>
#include <plog/Init.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Appenders/ColorConsoleAppender.h>

#include "../common/socket_io/socket.hpp"
#include "../server/session/session.hpp"
#include "session/session.hpp"

#define PORT 6999
#define ADDRESS "dtb-server"

using namespace std;

int main(int argc, char *argv[])
{

    char* username_ptr = argv[1];
    char* address_ptr = argv[2];
    char* port_ptr = argv[3];

    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::verbose, &consoleAppender);

    if (username_ptr == NULL) {
        PLOGE << "Error! No username provided. Exiting..." << endl;
        exit(1);
    }
    if (address_ptr == NULL) {
        PLOGE << "Error! No server address provided. Exiting..." << endl;
        exit(1);
    }
    if (port_ptr == NULL) {
        PLOGE << "Error! No server port provided. Exiting..." << endl;
        exit(1);
    }
    int server_port = (int)atoi(argv[3]);
    string server_addr(address_ptr);
    string username(username_ptr);
    unique_ptr<ClientSessionManager> manager(new ClientSessionManager(server_addr, server_port, username)); 
    // shared_ptr<Socket> socket(new Socket(server_addr_name, (int)server_port, Client, 1024));

    // SessionRequest *session = new SessionRequest(CommandPublisher, username);
    // uint8_t *bytes;
    // uint8_t **bytes_ptr = &bytes;
    // size_t sreq_size = session->to_bytes(bytes_ptr);
    // Packet *packet = new Packet(ClientSession, 1, sreq_size, sreq_size, bytes);
    // size_t packet_size = packet->to_bytes(bytes_ptr);
    // n = socket->send(bytes, packet_size, socket->socket_fd);

    // if (n < 0)
    //     printf("ERROR writing to socket\n");
    // free(packet);
    // bzero(buffer,1024);

    // while (0) {
    //     printf("Enter the message: ");
    //     bzero(buffer, 1024);
    //     fgets(buffer, 1024, stdin);
    //     uint16_t msg_size = strlen(buffer);
    //     uint16_t seq = 1;

    //     packet = new Packet(Command, seq, msg_size, msg_size, (uint8_t *)buffer);
    //     packet_size = packet->to_bytes(bytes_ptr);

    //     std::cout << "Generated " << (char *)bytes << std::endl;
    //     n = write(sockfd, bytes, packet_size);
    //     if (n < 0)
    //         printf("ERROR writing to socket\n");
    //     free(packet);
    //     bzero(buffer,1024);
    // }
    // int collected_bytes = 0;
    // int total_bytes = 1;
    // int iteration = 1;
    // FILE *file_output = fopen("output_file.txt", "wb");
    // while(collected_bytes < total_bytes) {
    //     n = socket->get_message_sync((uint8_t *)buffer, socket->socket_fd);
    //     if (n < 0)
    //         printf("ERROR reading from socket\n");
    //     cout << "Iteration: " << iteration << endl;
    //     iteration += 1;
    //     packet = new Packet((uint8_t *)buffer);
    //     fwrite(packet->payload, 1, packet->payload_size * sizeof(uint8_t), file_output);
    //     printf("Received packet type %d\n", packet->type);
    //     printf("File size: %d\n", packet->total_size);
    //     printf("Chunk size: %d\n", packet->payload_size);
    //     printf("Chunk index: %d\n", packet->seq_index);
    //     collected_bytes += packet->payload_size;
    //     cout << "Collected bytes: " << collected_bytes << endl;
    //     total_bytes = packet->total_size;
    //     free(packet);
    //     bzero(buffer, 1024);
    // }
    // fclose(file_output);

    return 0;
}
