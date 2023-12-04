#include "publisher.hpp"
#include <iostream>
#include <strings.h>
#include <sys/stat.h>
#include <memory>
#include "../file_io/file_io.hpp"
#include "../../common/vars.hpp"

using namespace std;


ServerEventPublisher::ServerEventPublisher(shared_ptr<ServerContext> context) {
    this->context = context;
}


void ServerEventPublisher::loop() {
    shared_ptr<Socket> socket = this->context->socket;
    int channel = this->context->connection->channel;
    shared_ptr<uint8_t> buffer((uint8_t *)malloc(BUFFER_SIZE));

    while(context->socket->get_message_async(buffer.get(), context->connection->channel) != 0) {
        cout << "Publisher waiting on channel " << channel << "..." << endl;
        sleep(1);
    }
}
