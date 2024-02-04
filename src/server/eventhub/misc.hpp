#ifndef EVT_MISC
#define EVT_MISC
#include "../../common/socket_io/socket.hpp"
#include "../../common/eventhub/models.hpp"

shared_ptr<Command> receive_command(shared_ptr<Socket> socket, int channel);


#endif
