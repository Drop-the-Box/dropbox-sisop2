#include "publisher.hpp"
#include <iostream>
#include <strings.h>
#include <sys/stat.h>
#include <memory>
#include "../file_io/file_io.hpp"

using namespace std;


EventPublisher::EventPublisher(shared_ptr<Socket> socket, Connection *connection, UserStore *storage) {
   this->socket = socket;
   this->connection = connection;
   this->storage = storage;
}

long get_file_size(string filename) {
    struct stat file_status;
    if (stat(filename.c_str(), &file_status) < 0) {
        return -1;
    }

    return file_status.st_size;
}

void EventPublisher::loop() {
    long file_size = get_file_size("sample_file.pdf");
    cout << "File size: " << file_size << endl;
    FILE *fp = fopen("sample_file.pdf", "rb");
    fseek(fp, 0, SEEK_SET);
    size_t bytes_read = 0;
    int seq_index = 1;
    int file_buf_size = 1014;
    int generated_bytes = 0;

    unique_ptr<FileMetadata> file_metadata(new FileMetadata("sample_file.pdf", "/code/"));

    shared_ptr<uint8_t> file_buf((uint8_t *)malloc(file_buf_size * sizeof(uint8_t)));
    buffer = (uint8_t *)malloc(socket->buffer_size);

    if (fp != NULL) {
        while((bytes_read = fread(file_buf.get(), sizeof (uint8_t), file_buf_size, fp)) > 0) {
            cout << "Chunk index: " << seq_index << endl;
            unique_ptr<Packet> packet(new Packet(FileChunk, seq_index, file_size, bytes_read, file_buf.get()));
            generated_bytes = packet->to_bytes(&buffer);
            cout << "Result: " << generated_bytes << endl;
            this->socket->send(buffer, generated_bytes, this->connection->channel);
            bzero(buffer, generated_bytes);
            bzero(file_buf.get(), bytes_read);
            seq_index += 1;
        }
    }
    free(buffer);
}
