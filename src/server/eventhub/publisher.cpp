#include "publisher.hpp"
#include "../file_io/file_io.hpp"
#include <iostream>
#include <sstream>
#include <memory>
#include <plog/Log.h>
#include <strings.h>
#include <sys/stat.h>
#include "../../common/file_io/file_io.hpp"
#include "./misc.hpp"

using namespace std;

ServerEventPublisher::ServerEventPublisher(shared_ptr<ServerContext> context) {
    this->context = context;
    this->socket = this->context->socket;
    this->channel = this->context->connection->channel;
    string username = this->context->device->username;
    ostringstream oss;
    shared_ptr<ReplicaManager> current_server = context->election_service->current_server;
    PLOGI << "current server PID: " << current_server->server_pid << endl;
    oss << FileHandler::get_sync_dir(username, DIR_SERVER, current_server); 
    this->sync_dir = oss.str();
}

void ServerEventPublisher::propagate_to_clients(shared_ptr<Command> command) {
    string username = this->context->device->username;
    map<string, Device *> user_devices = context->storage->get_user_devices(username);
    map<string, Device *>::iterator dev_iter;
    for (dev_iter = user_devices.begin(); dev_iter != user_devices.end() && dev_iter->first != username; dev_iter++) {
        Device *device = dev_iter->second;
        map<int, shared_ptr<Connection> > connections = device->connections;
        map<int, shared_ptr<Connection> >::iterator conn_it;
        for (conn_it = connections.begin(); conn_it != connections.end(); conn_it++) {
            shared_ptr<Connection> connection = conn_it->second;
            if(connection->address == context->connection->address) {
                continue;
            }
            SessionType session_type = conn_it->second->session_type;
            if (session_type == FileExchange && command->type == UploadFile) {
                uint8_t *command_bytes;
                int command_size = command->to_bytes(&command_bytes);
                PLOGW << "Notifying connection on channel " << connection->channel 
                    << "command of type " << command->type << endl;
                write(connection->pipe_fd[1], command_bytes, command_size);
            }
        }
    }
}

void ServerEventPublisher::loop() {
    shared_ptr<Socket> socket  = this->context->socket;
    int                channel = this->context->connection->channel;
    string username = this->context->device->username;
    unique_ptr<FileHandler> file_handler(new FileHandler(this->sync_dir));
    PLOGI << "------------- Connected to event publisher" << endl;
    while (!socket->has_error(channel) && !*socket->interrupt) {
        if(context->socket->has_event(channel)) {
            shared_ptr<Command> command = receive_command(socket, channel);
            if (command == NULL) {
                usleep(10000);
                continue;
            }
            PLOGI << "Received command '" << command->type << "' and arguments " << command->arguments << endl;
            if(command->type == UploadFile) {
                shared_ptr<FileMetadata> metadata = file_handler->receive_file(this->sync_dir, socket, channel);
                if (metadata == NULL) {
                    continue;
                }
                context->repl_service->send_file(username, metadata->name);
                PLOGI << "Received file on publisher. Notifying other devices" << endl;
            } else if (command->type == DeleteFile) {
                ostringstream file_path;
                file_handler->delete_file(command->arguments);
                context->repl_service->send_command(username, command);
            }
<<<<<<< Updated upstream
=======
            // this->propagate_to_clients(command);
            shared_ptr<Event> reply_evt(new Event(CommandSuccess, " Command propagated to all backups"));
            reply_evt->send(socket, channel);
>>>>>>> Stashed changes
            usleep(10000);

            PLOGI << "Publisher has received a file on channel " << channel << endl;
        }
    }
}
