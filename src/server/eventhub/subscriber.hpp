#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include "../session/models.hpp"
#include "../socket_io/socket.hpp"

#include <memory>

using namespace std;

class EventSubscriber {

    public:
        EventSubscriber(shared_ptr<Socket> socket, Connection *connection, UserStore *storage);
        void loop();
        void get_event();
};

#endif
