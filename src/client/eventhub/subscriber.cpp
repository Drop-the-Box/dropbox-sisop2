#include "subscriber.hpp"
#include <thread>

ClientSubscriber::ClientSubscriber(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    this->context = context;
    this->socket  = socket;
}

void ClientSubscriber::loop() {
    int channel = this->socket->socket_fd;
    while (!*socket->interrupt) {
        // PLOG_INFO << "Subscriber channel: " << channel << endl;
        // std::this_thread::sleep_for(std::chrono::seconds(10));
    }
};

void get_event(){

};
