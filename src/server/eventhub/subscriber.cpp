#include <stdlib.h>
#include <unistd.h>

#include "subscriber.hpp"


ServerEventSubscriber::ServerEventSubscriber(shared_ptr<ServerContext> context) {
    this->context = context;
}


void ServerEventSubscriber::loop() {
    cout << "------------- Connected to event subscriber" << endl;
    while(1) {
        sleep(1);
    }
}
