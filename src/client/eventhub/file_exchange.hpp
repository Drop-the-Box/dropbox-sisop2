#ifndef CLIENT_FILEX_H
#define CLIENT_FILEX_H

#include "../../common/file_io/file_io.hpp"
#include "../../common/socket_io/socket.hpp"
#include "../session/session.hpp"
#include <sys/types.h>

class ClientFileSync {
    shared_ptr<ClientContext> context;
    shared_ptr<Socket>        socket;
    shared_ptr<FileHandler>   file_handler;

public:
    ClientFileSync(shared_ptr<ClientContext> context, shared_ptr<Socket> socket);
    void loop();
    void get_event();
};

#endif
