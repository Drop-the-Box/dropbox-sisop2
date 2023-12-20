#include <plog/Log.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "../../common/vars.hpp"
#include "file_sync.hpp"

using namespace std;

FileSync::FileSync(shared_ptr<ServerContext> context) {
    this->context = context;
}

void FileSync::sync_all() {
    PLOGI << "-------------------- Connected to filesync" << endl;
    const string username = context->device->username;

    string         sync_dir = FileHandler::get_sync_dir(username, DIR_SERVER);
    vector<string> files    = FileHandler::list_files(sync_dir);

    vector<string>::iterator filename;
    unique_ptr<FileHandler> file_handler(new FileHandler(sync_dir));
    
    for (filename = files.begin(); filename != files.end(); filename++) {
        ostringstream oss;
        oss << sync_dir << "/" << *filename;
        string filepath = oss.str();
        PLOGD << "File: " << filepath << endl;
        file_handler->open(filepath);
        file_handler->send(context->socket, context->connection->channel);
    }
    PLOGI << "End of filesync server loop" << endl;
}

void FileSync::loop() {
    this->sync_all();
    shared_ptr<Socket>  socket  = this->context->socket;
    int                 channel = this->context->connection->channel;
    shared_ptr<uint8_t> buffer((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t)));
    while (context->socket->get_message_sync(buffer.get(), context->connection->channel) != 0) {
        PLOGI << "Filesync waiting on channel " << channel << "..." << endl;
        sleep(1);
    }
}
