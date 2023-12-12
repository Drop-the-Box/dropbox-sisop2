#ifndef CLIENT_SUB_H
#define CLIENT_SUB_H

#include "../../common/file_io/inotify.hpp"
#include "../../common/socket_io/socket.hpp"
#include "../session/session.hpp"
#include <memory>

class ClientSubscriber {
    shared_ptr<ClientContext> context;
    shared_ptr<Socket>        socket;

public:
    ClientSubscriber(shared_ptr<ClientContext> context, shared_ptr<Socket> socket);
    void loop();
    void get_event();
};

#endif
