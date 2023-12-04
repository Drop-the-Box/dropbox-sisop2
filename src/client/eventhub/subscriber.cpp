#include "subscriber.hpp"


ClientSubscriber::ClientSubscriber(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    this->context = context;
    this->socket = socket;
}


void ClientSubscriber::loop() {
    while(1);
};


void get_event() {

};
