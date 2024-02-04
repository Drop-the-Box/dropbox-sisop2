#ifndef CLIENT_SUB_H
#define CLIENT_SUB_H

#include "../session/session.hpp"
#include <memory>

class ClientSubscriber {
    shared_ptr<ClientContext> context;
    bool *interrupt;

public:
    ClientSubscriber(shared_ptr<ClientContext> context, bool *interrupt);
    void loop();
    void get_event();
};

#endif
