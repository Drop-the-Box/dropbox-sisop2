#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <memory>

#include "../../common/socket_io/socket.hpp"
#include "../userland/models.hpp"

using namespace std;

class ServerEventPublisher {
    uint8_t                  *buffer;
    shared_ptr<ServerContext> context;
    shared_ptr<Socket> socket;
    int channel;
    string sync_dir;

public:
    ServerEventPublisher(shared_ptr<ServerContext> context);
    void loop();
    void send_event();
    void propagate_to_clients(shared_ptr<Command> command);
};
#endif
