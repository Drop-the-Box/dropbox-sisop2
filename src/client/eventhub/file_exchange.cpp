#include <sstream>

#include "../../common/file_io/file_io.hpp"
#include "../../common/vars.hpp"

#include "file_exchange.hpp"

using namespace std;



ClientFileSync::ClientFileSync(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    this->context = context;
    this->socket = socket;
}


void ClientFileSync::loop() {
    char buffer[1024];
    int total_bytes = 0;
    int collected_bytes;
    int iteration;
    int chars_read;
    int file_no = 1;

    while((chars_read = socket->get_message_sync((uint8_t *)buffer, socket->socket_fd)) != 0) {
        collected_bytes = 0;
        iteration = 1;
        chars_read = 0;
        if (chars_read < 0)
            printf("ERROR reading from socket\n");
        Packet *packet = new Packet((uint8_t *)buffer);
        if (packet->type != FileMetadataMsg) {
            cerr << "Invalid packet type. Expected file metadata, received: " << packet->type << endl;
        }
        unique_ptr<FileMetadata> metadata(new FileMetadata(packet->payload));
        cout << "Received file " << metadata->name << endl;
        total_bytes = metadata->size;
        printf("File size: %ld\n", (long)total_bytes);

        ostringstream oss;
        oss << this->context->sync_dir << "/" << metadata->name;
        string filepath = oss.str();
        cout << "Storing file in " << filepath.c_str() << endl;
        FILE *file_output = fopen(filepath.c_str(), "wb");
        
        while(collected_bytes < total_bytes) {
            chars_read = socket->get_message_sync((uint8_t *)buffer, socket->socket_fd);
            cout << "Chars read: " << chars_read << endl;
            if (chars_read < 0)
                printf("ERROR reading from socket\n");
            cout << "Iteration: " << iteration << endl;
            iteration += 1;
            if (file_no == 2) {
                cout << "Second file: " << endl;
            }
            unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
            if (packet->type != FileChunk) {
                cerr << "Invalid file chunk: " << endl;
                cout << "Packet type: " << packet->type << endl;
                cout << "Packet seq idx: " << packet->seq_index << endl;
                cout << "Packet total size: " << packet->total_size << endl;
                cout << "Packet payload size: " << packet->payload_size << endl;
                cout << "Packet payload: " << packet->payload << endl;
                return;
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
            free(packet->payload);
        }
        if (file_output == NULL) {
            fclose(file_output);
        }
        file_no += 1;
    }
};
