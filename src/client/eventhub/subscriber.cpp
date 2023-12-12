#include "subscriber.hpp"
#include <thread>

ClientSubscriber::ClientSubscriber(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    this->context = context;
    this->socket  = socket;
}

void ClientSubscriber::loop() {
    int count   = 0;
    int channel = this->socket->socket_fd;
    while (!*socket->interrupt) {
        count++;
        // PLOG_INFO << "Subscriber loop count: " << count << endl;
        // PLOG_INFO << "Subscriber channel: " << channel << endl;
        // std::this_thread::sleep_for(std::chrono::seconds(5));
    }
};

void get_event(){

};
