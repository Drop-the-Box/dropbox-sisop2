#include "publisher.hpp"
#include "../../common/vars.hpp"
#include "../file_io/file_io.hpp"
#include "file_exchange.hpp"
#include <iostream>
#include <sstream>
#include <memory>
#include <plog/Log.h>
#include <strings.h>
#include <sys/stat.h>

using namespace std;

ServerEventPublisher::ServerEventPublisher(shared_ptr<ServerContext> context) {
    this->context = context;
}

void ServerEventPublisher::loop() {
    shared_ptr<Socket> socket  = this->context->socket;
    int                channel = this->context->connection->channel;
    uint8_t            buffer[BUFFER_SIZE];
    ostringstream oss;
    oss << "./srv_sync_dir/" << this->context->device->username;
    unique_ptr<FileHandler> file_handler(new FileHandler(oss.str()));
    PLOGI << "------------- Connected to event publisher" << endl;
    while (!socket->has_error(channel)) {
        shared_ptr<FileMetadata> metadata;
        file_handler->receive_file(oss.str(), metadata, socket, channel);
        PLOGI << "Publisher has received a file on channel " << channel << endl;
    }
}
