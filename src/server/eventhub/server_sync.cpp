#include "server_sync.hpp"
#include "../../common/vars.hpp"
#include "../../common/file_io/file_io.hpp"
#include <string.h>
#include <iostream>
#include <iterator>

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
        if (command == NULL) {
            usleep(10000);
            continue;
        }
        PLOGI << "Received command " << command->type << " from leader with args " << command->arguments << endl;
        if (command->type == UploadFile) {
            string username = string(strrchr(command->arguments.c_str(), ' '));
            username.erase(0);
            stringstream oss;
            oss << "./srv_sync_dir/" << username; 
            unique_ptr<FileHandler> file_handler(new FileHandler(oss.str()));
            shared_ptr<FileMetadata> metadata = NULL;
            metadata = file_handler->receive_file(oss.str(), socket, channel);
            if (metadata == NULL) {
                PLOGE << "Error receiving file from leader.";
                shared_ptr<Event> reply_evt(new Event(CommandFailed, "Failed receive file."));
                reply_evt->send(socket, channel);
                continue;
            }
            PLOGI << "Received file " << metadata->name << " for user " << username << " from leader." << endl;
            shared_ptr<Event> reply_evt(new Event(CommandSuccess, " Command propagated."));
            reply_evt->send(socket, channel);

        } else if (command->type == DeleteFile) {
        }
    }
    PLOGI << "Disconnected from backup subscriber" << endl;
}
