#include <plog/Log.h>
#include <sstream>

#include "../../common/file_io/file_io.hpp"
#include "../../common/vars.hpp"

#include "file_exchange.hpp"

using namespace std;

ClientFileSync::ClientFileSync(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    this->context = context;
    this->socket  = socket;
}

void ClientFileSync::loop() {
    char buffer[BUFFER_SIZE];
    int  total_bytes = 0;
    int  collected_bytes;
    int  iteration;
    int  chars_read;

    while ((chars_read = socket->get_message_sync((uint8_t *)buffer, socket->socket_fd)) != 0) {
        collected_bytes = 0;
        iteration       = 1;
        chars_read      = 0;
        if (chars_read < 0)
            printf("ERROR reading from socket\n");
        Packet *packet = new Packet((uint8_t *)buffer);
        if (packet->type != FileMetadataMsg) {
            PLOGE << "Invalid packet type. Expected file metadata, received: " << packet->type << endl;
        }
        unique_ptr<FileMetadata> metadata(new FileMetadata(packet->payload));
        PLOGI << "Received file " << metadata->name << endl;
        total_bytes = metadata->size;
        printf("File size: %ld\n", (long)total_bytes);

        ostringstream oss;
        oss << this->context->sync_dir << "/" << metadata->name;
        string filepath = oss.str();
        PLOGI << "Storing file in " << filepath.c_str() << endl;
        FILE *file_output = fopen(filepath.c_str(), "wb");

        while (collected_bytes < total_bytes) {
            chars_read = socket->get_message_sync((uint8_t *)buffer, socket->socket_fd);
            PLOGD << "Chars read: " << chars_read << endl;
            if (chars_read < 0) {
                printf("ERROR reading from socket\n");
                break;
            }
            PLOGD << "Iteration: " << iteration << endl;
            iteration += 1;
            unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
            if (packet->type != FileChunk) {
                PLOGE << "Invalid file chunk: " << endl;
                PLOGD << "Packet type: " << packet->type << endl;
                PLOGD << "Packet seq idx: " << packet->seq_index << endl;
                PLOGD << "Packet total size: " << packet->total_size << endl;
                PLOGD << "Packet payload size: " << packet->payload_size << endl;
                PLOGD << "Packet payload: " << packet->payload << endl;
                return;
            }
            fwrite(packet->payload, 1, packet->payload_size * sizeof(uint8_t), file_output);
            PLOGD << "Received packet type " << packet->type << endl;
            PLOGD << "File size: " << (long)packet->total_size << endl;
            PLOGD << "Chunk size: " << packet->payload_size << endl;
            PLOGD << "Chunk index: " << packet->seq_index << endl;
            collected_bytes += packet->payload_size;
            PLOGD << "Collected bytes: " << collected_bytes << endl;
            total_bytes = packet->total_size;
            bzero(buffer, BUFFER_SIZE);
        }
        if (file_output != NULL) {
            fclose(file_output);
        }
    }
};
