#include "server_sync.hpp"
#include "../../common/vars.hpp"

BackupSubscriber::BackupSubscriber(shared_ptr<ServerContext> context) {
    this->context = context;
    this->socket = context->socket;
    this->channel = context->connection->channel;
    this->interrupt = context->socket->interrupt;
}

shared_ptr<Command> BackupSubscriber::receive_command() {
    char buffer[BUFFER_SIZE];
    while(true) {
        try {
            socket->get_message_sync((uint8_t *)buffer, channel, true);
            break;
        } catch (SocketTimeoutError &exc) {};
    }
    PLOGI << "Received something on channel " << channel << endl;
    unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
    if (packet->type == CommandMsg) {
        shared_ptr<Command> command(new Command(packet->payload));
        return command;
    };
    return NULL;
}

void BackupSubscriber::loop() {
    while (!socket->has_error(channel) && !*interrupt) {
        PLOGI << "Awaiting for command..." << endl;
        shared_ptr<Command> command = this->receive_command();
        if (command != NULL) {
            PLOGI << "Received command " << command->type << " from leader with args " << command->arguments << endl;
        } else {
        }
        usleep(10000);
    }
    PLOGI << "Disconnected from backup subscriber" << endl;
}
