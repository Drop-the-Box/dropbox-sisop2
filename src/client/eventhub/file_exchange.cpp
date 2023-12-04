#include <sstream>

#include "../../common/file_io/file_io.hpp"

#include "file_exchange.hpp"

using namespace std;



ClientFileSync::ClientFileSync(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    this->context = context;
    this->socket = socket;
}


void ClientFileSync::loop() {
    int collected_bytes = 0;
    int total_bytes = 1;
    int iteration = 1;
    char buffer[1024];

    int n = socket->get_message_sync((uint8_t *)buffer, socket->socket_fd);
    if (n < 0)
        printf("ERROR reading from socket\n");
    unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
    if (packet->type != FileMetadataMsg) {
    }
    unique_ptr<FileMetadata> metadata(new FileMetadata(packet->payload));
    cout << "Received file " << metadata->name << endl;

    ostringstream oss;
    oss << this->context->sync_dir << "/" << metadata->name;
    string filepath = oss.str();
    cout << "Storing file in " << filepath.c_str() << endl;
    FILE *file_output = fopen(filepath.c_str(), "wb");
    
    while(collected_bytes < total_bytes) {
        n = socket->get_message_sync((uint8_t *)buffer, socket->socket_fd);
        if (n < 0)
            printf("ERROR reading from socket\n");
        cout << "Iteration: " << iteration << endl;
        iteration += 1;
        unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
        if (packet->type != FileChunk) {
            cerr << "Invalid file chunk: " << endl;
            throw;
        }
        fwrite(packet->payload, 1, packet->payload_size * sizeof(uint8_t), file_output);
        printf("Received packet type %d\n", packet->type);
        printf("File size: %ld\n", (long)packet->total_size);
        printf("Chunk size: %d\n", packet->payload_size);
        printf("Chunk index: %d\n", packet->seq_index);
        collected_bytes += packet->payload_size;
        cout << "Collected bytes: " << collected_bytes << endl;
        total_bytes = packet->total_size;
        bzero(buffer, 1024);
    }
    fclose(file_output);
    while(1);
};
