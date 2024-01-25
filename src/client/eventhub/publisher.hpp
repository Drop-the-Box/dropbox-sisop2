#ifndef CLIENT_PUB_H
#define CLIENT_PUB_H

#include "../session/session.hpp"
#include <memory>

using namespace std;

class ClientPublisher {
    shared_ptr<ClientContext> context;
    bool *interrupt;

public:
    ClientPublisher(shared_ptr<ClientContext> context, bool *interrupt);
    void loop();
    void send_event();
};

#endif
