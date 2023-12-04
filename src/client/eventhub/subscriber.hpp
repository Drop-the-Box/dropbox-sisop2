#ifndef CLIENT_SUB_H
#define CLIENT_SUB_H

#include <memory>
#include "../session/session.hpp"
#include "../../common/socket_io/socket.hpp"

class ClientSubscriber {
    shared_ptr<ClientContext> context;
    shared_ptr<Socket> socket;

    public:
        ClientSubscriber(shared_ptr<ClientContext> context, shared_ptr<Socket> socket);
        void loop();
        void get_event();
};

#endif
