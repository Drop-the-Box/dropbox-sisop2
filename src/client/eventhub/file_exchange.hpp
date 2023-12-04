#ifndef CLIENT_FILEX_H
#define CLIENT_FILEX_H

#include <memory>
#include "../session/session.hpp"
#include "../../common/socket_io/socket.hpp"

class ClientFileSync {
    shared_ptr<ClientContext> context;
    shared_ptr<Socket> socket;

    public:
        ClientFileSync(shared_ptr<ClientContext> context, shared_ptr<Socket> socket);
        void loop();
        void get_event();
};


#endif
