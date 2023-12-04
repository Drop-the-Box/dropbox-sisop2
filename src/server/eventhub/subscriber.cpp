#include <stdlib.h>
#include <unistd.h>

#include "subscriber.hpp"
#include "../../common/vars.hpp"


ServerEventSubscriber::ServerEventSubscriber(shared_ptr<ServerContext> context) {
    this->context = context;
}


void ServerEventSubscriber::loop() {
    cout << "------------- Connected to event subscriber" << endl;
    shared_ptr<Socket> socket = this->context->socket;
    int channel = this->context->connection->channel;
    shared_ptr<uint8_t> buffer((uint8_t *)malloc(BUFFER_SIZE));

    while(context->socket->get_message_async(buffer.get(), context->connection->channel) != 0) {
        cout << "Subscriber waiting on channel " << channel << "..." << endl;
        sleep(1);
    }
}
