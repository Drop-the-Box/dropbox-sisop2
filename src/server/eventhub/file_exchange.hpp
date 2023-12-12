#ifndef SERVER_FILEX_H
#define SERVER_FILEX_H

#include "../../common/socket_io/socket.hpp"
#include "../session/session.hpp"
#include "../userland/models.hpp"
#include <memory>

class ServerFileSync {
    shared_ptr<ServerContext> context;
    shared_ptr<Socket>        socket;

public:
    ServerFileSync(shared_ptr<ServerContext> context, shared_ptr<Socket> socket);
    void loop();
    void get_event();
};

#endif