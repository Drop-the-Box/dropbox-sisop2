#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include "../../common/socket_io/socket.hpp"
#include "../session/models.hpp"
#include "../userland/models.hpp"
#include "file_exchange.hpp"

#include <memory>

using namespace std;

class ServerEventSubscriber {
    shared_ptr<ServerContext> context;

public:
    ServerEventSubscriber(shared_ptr<ServerContext> context);
    void loop();
    void get_event();
};

#endif
