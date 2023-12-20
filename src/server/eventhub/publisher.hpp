#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <memory>

#include "../../common/socket_io/socket.hpp"
#include "../session/models.hpp"
#include "../userland/models.hpp"
#include "../../common/file_io/file_io.hpp"

using namespace std;

class ServerEventPublisher {
    uint8_t                  *buffer;
    shared_ptr<ServerContext> context;

public:
    ServerEventPublisher(shared_ptr<ServerContext> context);
    void loop();
    void send_event();
};
#endif
