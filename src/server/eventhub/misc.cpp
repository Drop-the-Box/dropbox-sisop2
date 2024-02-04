#include "misc.hpp"
#include "../../common/vars.hpp"

shared_ptr<Command> receive_command(shared_ptr<Socket> socket, int channel) {
    char buffer[BUFFER_SIZE];
    while(true) {
        try {
            socket->get_message_sync((uint8_t *)buffer, channel, true);
            break;
        } catch (SocketTimeoutError &exc) {};
    }
    unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
    if (packet->type == CommandMsg) {
        shared_ptr<Command> command(new Command(packet->payload));
        return command;
    };
    return NULL;
}
