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
    // PLOGI << "Subscriber has event on channel " << channel << endl;
    int channel = context->connection->channel;
    shared_ptr<Socket> socket = context->socket;
    while (!socket->has_error(channel) && !*socket->interrupt) {
        usleep(10000);
    }
}
