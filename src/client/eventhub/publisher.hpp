#ifndef CLIENT_PUB_H
#define CLIENT_PUB_H

#include "../../common/socket_io/socket.hpp"
#include "../session/session.hpp"
#include <memory>

using namespace std;

class ClientPublisher {
    shared_ptr<ClientContext> context;
    shared_ptr<Socket>        socket;

public:
    ClientPublisher(shared_ptr<ClientContext> context, shared_ptr<Socket> socket);
    void loop();
    void send_event();
};

#endif
