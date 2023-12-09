#include "publisher.hpp"
#include <iostream>
#include <strings.h>
#include <sys/stat.h>
#include <memory>
#include "../file_io/file_io.hpp"
#include "../../common/vars.hpp"
#include <plog/Log.h>

using namespace std;


ServerEventPublisher::ServerEventPublisher(shared_ptr<ServerContext> context) {
    this->context = context;
}


void ServerEventPublisher::loop() {
    shared_ptr<Socket> socket = this->context->socket;
    int channel = this->context->connection->channel;
    uint8_t buffer[BUFFER_SIZE];
    PLOGI << "------------- Connected to event publisher" << endl;

    while(!context->socket->has_event(channel)) {
        PLOGI << "Publisher waiting on channel " << channel << "..." << endl;
        sleep(1);
    }
}
