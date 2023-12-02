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

#include "../server/socket_io/socket.hpp"
#include "../server/session/session.hpp"

#define PORT 6999
#define ADDRESS "dtb-server"

int main(int argc, char *argv[])
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char *server_addr = argv[2];
    uint16_t server_port = atoi(argv[3]);
    char *username = argv[1];

    char buffer[1024];
	server = gethostbyname(server_addr);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        printf("ERROR opening socket\n");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(server_port);
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);


	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        printf("ERROR connecting\n");

    SessionRequest *session = new SessionRequest(CommandPublisher, username);
    uint8_t *bytes;
    uint8_t **bytes_ptr = &bytes;
    size_t sreq_size = session->to_bytes(bytes_ptr);
    Packet *packet = new Packet(ClientSession, 1, sreq_size, sreq_size, bytes);
    size_t packet_size = packet->to_bytes(bytes_ptr);
    n = write(sockfd, bytes, packet_size);
    if (n < 0)
        printf("ERROR writing to socket\n");
    free(packet);
    bzero(buffer,1024);

    while (0) {
        printf("Enter the message: ");
        bzero(buffer, 1024);
        fgets(buffer, 1024, stdin);
        uint16_t msg_size = strlen(buffer);
        uint16_t seq = 1;

        packet = new Packet(Command, seq, msg_size, msg_size, (uint8_t *)buffer);
        packet_size = packet->to_bytes(bytes_ptr);

        std::cout << "Generated " << (char *)bytes << std::endl;
        n = write(sockfd, bytes, packet_size);
        if (n < 0)
            printf("ERROR writing to socket\n");
        free(packet);
        bzero(buffer,1024);
    }
    int collected_bytes = 0;
    int total_bytes = 1;
    int iteration = 1;
    FILE *file_output = fopen("output_file.txt", "wb");
    while(collected_bytes < total_bytes) {
        n = recv(sockfd, buffer, 1024, 0);
        if (n < 0)
            printf("ERROR reading from socket\n");
        cout << "Iteration: " << iteration << endl;
        iteration += 1;
        packet = new Packet((uint8_t *)buffer);
        fwrite(packet->payload, 1, packet->payload_size * sizeof(uint8_t), file_output);
        printf("Received packet type %d\n", packet->type);
        printf("File size: %d\n", packet->total_size);
        printf("Chunk size: %d\n", packet->payload_size);
        printf("Chunk index: %d\n", packet->seq_index);
        collected_bytes += packet->payload_size;
        cout << "Collected bytes: " << collected_bytes << endl;
        total_bytes = packet->total_size;
        free(packet);
        bzero(buffer, 1024);
        // printf("%s\n",buffer);
    }
    fclose(file_output);
	/* read from the socket */

	close(sockfd);
    return 0;
}
