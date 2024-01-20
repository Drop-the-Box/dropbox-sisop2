#include "publisher.hpp"
#include "../../common/vars.hpp"
#include "../file_io/file_io.hpp"
#include <iostream>
#include <sstream>
#include <memory>
#include <plog/Log.h>
#include <strings.h>
#include <sys/stat.h>

using namespace std;

ServerEventPublisher::ServerEventPublisher(shared_ptr<ServerContext> context) {
    this->context = context;
}

void ServerEventPublisher::loop() {
    shared_ptr<Socket> socket  = this->context->socket;
    int                channel = this->context->connection->channel;
    uint8_t            buffer[BUFFER_SIZE];
    string username = this->context->device->username;
    ostringstream oss;
    oss << "./srv_sync_dir/" << username; 
    unique_ptr<FileHandler> file_handler(new FileHandler(oss.str()));
    PLOGI << "------------- Connected to event publisher" << endl;
    while (!socket->has_error(channel)) {
        if(context->socket->has_event(channel)) {
            shared_ptr<FileMetadata> metadata;
            file_handler->receive_file(oss.str(), metadata, socket, channel);
            map<string, Device *> user_devices = context->storage->get_user_devices(username);
            map<string, Device *>::iterator dev_iter;
            for (dev_iter = user_devices.begin(); dev_iter != user_devices.end() && dev_iter->first != username; dev_iter++) {
                Device *device = dev_iter->second;
                map<int, shared_ptr<Connection> > connections = device->connections;
                map<int, shared_ptr<Connection> >::iterator conn_it;
                for (conn_it = connections.begin(); conn_it != connections.end(); conn_it++) {
                    shared_ptr<Connection> connection = conn_it->second;
                    if (conn_it->second->session_type == FileExchange) {
                        ostringstream file_path;
                        file_path << "./srv_sync_dir" << username << metadata->name;
                        int path_size = file_path.str().length() + 1;
                        char *fpath = (char *)malloc(path_size);
                        memcpy(fpath, file_path.str().c_str(), path_size);
                        PLOGW << "Notifying connection on channel " << connection->channel << " about file" << fpath;
                        write(connection->pipe_fd[1], fpath, strlen(fpath));
                    }
                }
            }

            PLOGI << "Publisher has received a file on channel " << channel << endl;
        }
    }
}
