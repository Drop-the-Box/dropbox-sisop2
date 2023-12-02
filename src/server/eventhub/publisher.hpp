#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <memory>

#include "../session/models.hpp"
#include "../socket_io/socket.hpp"

using namespace std;

class EventPublisher {
    Connection *connection;
    shared_ptr<Socket> socket;
    UserStore *storage;
    uint8_t *buffer;

    public:
        EventPublisher(shared_ptr<Socket> socket, Connection *connection, UserStore *storage);
        void loop();
        void send_event();
};
#endif
