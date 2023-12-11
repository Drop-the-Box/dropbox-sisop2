#ifndef CLIENT_PUB_H
#define CLIENT_PUB_H

#include <memory>
#include "../session/session.hpp"
#include "../../common/socket_io/socket.hpp"
#include "../../common/file_io/inotify.hpp"

using namespace std;


class ClientPublisher {
    shared_ptr<ClientContext> context;
    shared_ptr<Socket> socket;

    public:
        ClientPublisher(shared_ptr<ClientContext> context, shared_ptr<Socket> socket);
        void loop();
        void send_event();
};


#endif
