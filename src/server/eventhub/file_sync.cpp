#include <plog/Log.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "../../common/file_io/file_io.hpp"
#include "file_sync.hpp"
#include "./misc.hpp"

using namespace std;

FileSync::FileSync(shared_ptr<ServerContext> context) {
    this->context = context;
}

void FileSync::sync_all() {
    PLOGI << "-------------------- Connected to filesync" << endl;
    const string username = context->device->username;
    shared_ptr<ReplicaManager> current_server = context->election_service->current_server;
    string         sync_dir = FileHandler::get_sync_dir(username, DIR_SERVER, current_server);
    vector<string> files    = FileHandler::list_files(sync_dir);

    vector<string>::iterator filename;
    unique_ptr<FileHandler> file_handler(new FileHandler(sync_dir));
    int channel = context->connection->channel;
    shared_ptr<Socket> socket = context->socket;

    shared_ptr<Command> command = receive_command(socket, channel); 
    if (command->type == SyncDir) {
        PLOGI << "Starting filesync cycle" << endl;
        for (filename = files.begin(); filename != files.end(); filename++) {
            ostringstream oss;
            oss << sync_dir << "/" << *filename;
            string filepath = oss.str();
            PLOGD << "File: " << filepath << endl;
            file_handler->open(filepath);
            file_handler->send(socket, channel);
        }
        unique_ptr<Event> end_sync_evt(new Event(SyncDirFinished, "Sync complete!"));
        end_sync_evt->send(socket, channel);
    
        PLOGI << "End of filesync cycle" << endl;
    }
}

void FileSync::loop() {
    sleep(5);
    this->sync_all();
    int channel = context->connection->channel;
    shared_ptr<Socket> socket = context->socket;
    while (!socket->has_error(channel) && !*socket->interrupt) {
        sleep(1);
    }
    // shared_ptr<Socket>  socket  = this->context->socket;
    // int                 channel = this->context->connection->channel;
    // shared_ptr<uint8_t> buffer((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t)));
    // // close(context->connection->pipe_fd[1]);
    // // read(context->connection->pipe_fd[0], buffer.get(), BUFFER_SIZE - 1);
    // PLOGI << "Received filename " << buffer << endl;
    // while (context->socket->get_message_sync(buffer.get(), context->connection->channel) != 0) {
    //     PLOGI << "Filesync waiting on channel " << channel << "..." << endl;
    //     sleep(1);
    // }
}
