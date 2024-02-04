#include <memory>
#include <plog/Log.h>

#include "file_exchange.hpp"
#include "../../common/vars.hpp"

using namespace std;

ClientFileSync::ClientFileSync(shared_ptr<ClientContext> context, bool *interrupt) {
    this->context = context;
    this->interrupt = interrupt;
    string sync_dir = this->context->sync_dir;
    this->file_handler = make_shared<FileHandler>(sync_dir);
    ConnectionManager *conn_manager = context->conn_manager;

    shared_ptr<Command> sync_command = make_shared<Command>(SyncDir, "sync all files");
    conn_manager->send_command(sync_command, FileExchange);
}

void ClientFileSync::loop() {
    string sync_dir = this->context->sync_dir;
    ConnectionManager *conn_manager = context->conn_manager;
    shared_ptr<FileMetadata> metadata;
    char buffer[BUFFER_SIZE];
    while (!*this->interrupt && !conn_manager->has_error(FileExchange)) {
        try {
            conn_manager->get_message((uint8_t *)buffer, FileExchange);
            unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
            if (packet->type == EventMsg) {
                unique_ptr<Event> event(new Event(packet->payload));
                if (event->type == SyncDirFinished) {
                    *context->is_listening_dir = true;
                    PLOGI << "End of get_sync_dir" << endl;
                }
            }
            else if (packet->type == FileMetadataMsg) {
                shared_ptr<FileMetadata> metadata = make_shared<FileMetadata>(packet->payload);
                if (!context->file_monitor->add_file(metadata)) {
                    PLOGI << "File " << metadata->name << " already exists. Ignoring data..." << endl;
                    int bytes_count = 0;
                    while(bytes_count < metadata->size) {
                        conn_manager->get_message((uint8_t *)buffer, FileExchange);
                        unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
                        bytes_count += packet->payload_size;
                    }
                } else {
                    PLOGI << "Downloading file " << metadata->name  << " from the server..." << endl;
                    file_handler->receive_file(metadata, sync_dir, conn_manager, context->session_type);
                }
            }
        } catch (ConnectionResetError &exc) {
            continue;
        } catch (SocketTimeoutError &exc) {
            continue;
        }
        if (metadata == NULL) {
            continue;
        }
        PLOGI << "Received file in FileSync: " << metadata->name << endl;
    }
    // char buffer[BUFFER_SIZE];
    // int  total_bytes = 0;
    // int  collected_bytes;
    // int  iteration;

    // int chars_read = socket->get_message_sync((uint8_t *)buffer, socket->socket_fd);

    // while (chars_read != 0) {
    //     collected_bytes = 0;
    //     iteration       = 1;
    //     chars_read      = 0;
    //     if (chars_read < 0)
    //         printf("ERROR reading from socket\n");
    //     Packet *packet = new Packet((uint8_t *)buffer);
    //     if (packet->type != FileMetadataMsg) {
    //         PLOGE << "Invalid packet type. Expected file metadata, received: " << packet->type << endl;
    //         break;
    //     }
    //     unique_ptr<FileMetadata> metadata(new FileMetadata(packet->payload));
    //     PLOGI << "Received file " << metadata->name << endl;
    //     total_bytes = metadata->size;
    //     printf("File size: %ld\n", (long)total_bytes);

    //     ostringstream oss;
    //     oss << this->context->sync_dir << "/" << metadata->name;
    //     string filepath = oss.str();
    //     PLOGI << "Storing file in " << filepath.c_str() << endl;
    //     FILE *file_output = fopen(filepath.c_str(), "wb");

    //     while (collected_bytes < total_bytes) {
    //         chars_read = socket->get_message_sync((uint8_t *)buffer, socket->socket_fd);
    //         PLOGD << "Chars read: " << chars_read << endl;
    //         if (chars_read < 0) {
    //             printf("ERROR reading from socket\n");
    //             break;
    //         }
    //         PLOGD << "Iteration: " << iteration << endl;
    //         iteration += 1;
    //         unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
    //         if (packet->type != FileChunk) {
    //             PLOGE << "Invalid file chunk: " << endl;
    //             PLOGD << "Packet type: " << packet->type << endl;
    //             PLOGD << "Packet seq idx: " << packet->seq_index << endl;
    //             PLOGD << "Packet total size: " << packet->total_size << endl;
    //             PLOGD << "Packet payload size: " << packet->payload_size << endl;
    //             PLOGD << "Packet payload: " << packet->payload << endl;
    //             return;
    //         }
    //         fwrite(packet->payload, 1, packet->payload_size * sizeof(uint8_t), file_output);
    //         PLOGD << "Received packet type " << packet->type << endl;
    //         PLOGD << "File size: " << (long)packet->total_size << endl;
    //         PLOGD << "Chunk size: " << packet->payload_size << endl;
    //         PLOGD << "Chunk index: " << packet->seq_index << endl;
    //         collected_bytes += packet->payload_size;
    //         PLOGD << "Collected bytes: " << collected_bytes << endl;
    //         total_bytes = packet->total_size;
    //         bzero(buffer, BUFFER_SIZE);
    //     }
    //     if (file_output != NULL) {
    //         fclose(file_output);
    //     }
    //     chars_read = socket->get_message_sync((uint8_t *)buffer, socket->socket_fd);
    // }
};
