#include "server_sync.hpp"
#include "../../common/vars.hpp"
#include "../../common/file_io/file_io.hpp"
#include <string.h>
#include <iostream>
#include <pthread.h>

BackupSubscriber::BackupSubscriber(shared_ptr<ServerContext> context) {
    this->context = context;
    this->socket = context->socket;
    this->channel = context->connection->channel;
    this->interrupt = context->socket->interrupt;
    ServerElectionService *election_svc = context->election_service;
    pthread_create(&election_svc->leader_pooling_tid, NULL, election_svc->check_leader, election_svc);
    pthread_detach(election_svc->leader_pooling_tid);
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
    shared_ptr<ReplicaManager> current_server = context->election_service->current_server;

    while (!socket->has_error(channel) && !*interrupt) {
        PLOGI << "Awaiting for command..." << endl;
        shared_ptr<Command> command = this->receive_command();
        if (command == NULL) {
            usleep(10000);
            continue;
        }
        PLOGI << "Received command " << command->type << " from leader with args " << command->arguments << endl;
        std::string filename, username;
        std::stringstream s(command->arguments);
        s >> username >> filename;
        string sync_dir = FileHandler::get_sync_dir(username, DIR_SERVER, current_server); 
        PLOGI << "Sync dir is: " << sync_dir << endl;
        unique_ptr<FileHandler> file_handler(new FileHandler(sync_dir));

        if (command->type == UploadFile) {
            shared_ptr<FileMetadata> metadata = NULL;
            metadata = file_handler->receive_file(sync_dir, socket, channel);
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
            file_handler->delete_file(filename);
        }
    }
    PLOGI << "Disconnected from backup subscriber" << endl;
}
