#include <stdlib.h>
#include <unistd.h>
#include <plog/Log.h>

#include "subscriber.hpp"
#include "../../common/vars.hpp"


ServerEventSubscriber::ServerEventSubscriber(shared_ptr<ServerContext> context) {
    this->context = context;
}


void ServerEventSubscriber::loop() {
    PLOGI << "------------- Connected to event subscriber" << endl;
    shared_ptr<Socket> socket = this->context->socket;
    int channel = this->context->connection->channel;
    uint8_t buffer[BUFFER_SIZE];

    while(!context->socket->has_event(channel)) {
        PLOGI << "Subscriber waiting on channel " << channel << "..." << endl;
        sleep(1);
    }
}
