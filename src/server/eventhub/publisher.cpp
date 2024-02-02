#include "publisher.hpp"
#include "../../common/vars.hpp"
#include "../file_io/file_io.hpp"
#include <iostream>
#include <sstream>
#include <memory>
#include <plog/Log.h>
#include <strings.h>
#include <sys/stat.h>
#include "../../common/file_io/file_io.hpp"

using namespace std;

ServerEventPublisher::ServerEventPublisher(shared_ptr<ServerContext> context) {
    this->context = context;
    this->socket = this->context->socket;
    this->channel = this->context->connection->channel;
}

shared_ptr<Command> ServerEventPublisher::receive_command() {
    char buffer[BUFFER_SIZE];
    socket->get_message_sync((uint8_t *)buffer, channel, true);
    unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
    if (packet->type == CommandMsg) {
        shared_ptr<Command> command(new Command(packet->payload));
        return command;
    };
    return NULL;
}

void ServerEventPublisher::loop() {
    shared_ptr<Socket> socket  = this->context->socket;
    int                channel = this->context->connection->channel;
    string username = this->context->device->username;
    ostringstream oss;
    oss << "./srv_sync_dir/" << username; 
    unique_ptr<FileHandler> file_handler(new FileHandler(oss.str()));
    PLOGI << "------------- Connected to event publisher" << endl;
    while (!socket->has_error(channel) && !*socket->interrupt) {
        if(context->socket->has_event(channel)) {
            shared_ptr<Command> command = this->receive_command();
            if (command == NULL) {
                usleep(10000);
                continue;
            }
            PLOGI << "Received command '" << command->type << "' and arguments " << command->arguments << endl;
            if(command->type == UploadFile) {
                shared_ptr<FileMetadata> metadata = file_handler->receive_file(oss.str(), socket, channel);
                if (metadata == NULL) {
                    shared_ptr<Event> reply_evt(new Event(CommandFailed, "Failed receive file."));
                    reply_evt->send(socket, channel);
                    continue;
                }
                PLOGI << "Received file on publisher. Notifying other devices" << endl;
                context->repl_service->send_file(username, metadata->name);
            } else if (command->type == DeleteFile) {
                context->repl_service->send_command(username, command);
                // impl delete
            }
            shared_ptr<Event> reply_evt(new Event(CommandSuccess, " Command propagated to all backups"));
            reply_evt->send(socket, channel);
            usleep(10000);
            // map<string, Device *> user_devices = context->storage->get_user_devices(username);
            // map<string, Device *>::iterator dev_iter;
            // for (dev_iter = user_devices.begin(); dev_iter != user_devices.end() && dev_iter->first != username; dev_iter++) {
            //     Device *device = dev_iter->second;
            //     map<int, shared_ptr<Connection> > connections = device->connections;
            //     map<int, shared_ptr<Connection> >::iterator conn_it;
            //     for (conn_it = connections.begin(); conn_it != connections.end(); conn_it++) {
            //         shared_ptr<Connection> connection = conn_it->second;
            //         if (conn_it->second->session_type == FileExchange) {
            //             ostringstream file_path;
            //             file_path << "./srv_sync_dir" << username << metadata->name;
            //             int path_size = file_path.str().length() + 1;
            //             char *fpath = (char *)malloc(path_size);
            //             memcpy(fpath, file_path.str().c_str(), path_size);
            //             PLOGW << "Notifying connection on channel " << connection->channel << " about file" << fpath;
            //             // write(connection->pipe_fd[1], fpath, strlen(fpath));
            //             free(fpath);
            //         }
            //     }
            // }

            PLOGI << "Publisher has received a file on channel " << channel << endl;
        }
    }
}
