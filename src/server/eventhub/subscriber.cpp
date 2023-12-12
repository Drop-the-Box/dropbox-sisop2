#include <plog/Log.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../common/vars.hpp"
#include "subscriber.hpp"

ServerEventSubscriber::ServerEventSubscriber(shared_ptr<ServerContext> context) {
    this->context = context;
}

void ServerEventSubscriber::loop() {
    // PLOGI << "------------- Connected to event subscriber" << endl;
    shared_ptr<Socket> socket  = this->context->socket;
    int                channel = this->context->connection->channel;
    uint8_t            buffer[BUFFER_SIZE];
    // PLOGI << "Subscriber has event on channel " << channel << endl;
    while (!context->socket->has_event(channel)) {
        // PLOGI << "Subscriber waiting on channel " << channel << "..." << endl;
        // PLOGI << "Has event on channel " << channel << ": " << context->socket->has_event(channel) << endl;
        sleep(5);
    }
}
