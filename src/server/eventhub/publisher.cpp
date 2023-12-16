#include "publisher.hpp"
#include "../../common/vars.hpp"
#include "../file_io/file_io.hpp"
#include "file_exchange.hpp"
#include <iostream>
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
    // PLOGI << "------------- Connected to event publisher" << endl;
    // PLOGI << "Publisher has event on channel " << channel << endl;
    while (true) {
        if (!context->socket->has_event(channel)) {
            PLOGW << "Publisher waiting on channel " << channel << "..." << endl;
            sleep(1);
        } else {
            PLOGI << "Subscriber received event on channel " << channel << endl;
            PLOGW << "Has event on channel " << channel << ": " << context->socket->has_event(channel) << endl;
            int bytes = socket->receive(buffer, channel);
            if (bytes < 0) {
                PLOGE << "Error reading from socket" << endl;
                return;
            }
            PLOGI << "Subscriber received " << bytes << " bytes" << endl;
            unique_ptr<ServerFileSync> file_sync(new ServerFileSync(context, socket));
            file_sync->loop();
            PLOGI << "Subscriber received: " << buffer << endl;
            PLOGW << "End of subscriber server loop" << endl;
        }
    }
}
