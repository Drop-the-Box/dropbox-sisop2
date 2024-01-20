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
        //count++;
        // PLOG_INFO << "Subscriber loop count: " << count << endl;
        // PLOGI << "Subscriber channel: " << channel << endl;
        // sleep(5);
        // std::this_thread::sleep_for(std::chrono::seconds(5));
    }
};

void get_event(){

};
