#include "publisher.hpp"


ClientPublisher::ClientPublisher(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    this->context = context;
    this->socket = socket;
}


void ClientPublisher::loop() {
    while(1);
};


void ClientPublisher::send_event() {

};
