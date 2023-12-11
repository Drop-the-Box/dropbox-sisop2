#include "publisher.hpp"

ClientPublisher::ClientPublisher(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    this->context = context;
    this->socket  = socket;
}

void ClientPublisher::loop() {
    shared_ptr<Inotify> inotify = make_shared<Inotify>(this->context, this->socket);
    while (!*socket->interrupt) {
        inotify->read_event();
    }
};

void ClientPublisher::send_event(){

};
