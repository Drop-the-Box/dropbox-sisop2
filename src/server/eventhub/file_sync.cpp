#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
#include <plog/Log.h>

#include "file_sync.hpp"
#include "../../common/vars.hpp"

using namespace std;

FileSync::FileSync(shared_ptr<ServerContext> context) {
    this->context = context;
}

void FileSync::sync_all() {
    PLOGI << "-------------------- Connected to filesync" << endl;
    const string username = context->device->username;

    string sync_dir = FileHandler::get_sync_dir(username, DIR_SERVER);
    vector<string> files = FileHandler::list_files(sync_dir);

    vector<string>::iterator filename;
    for (filename=files.begin(); filename != files.end(); filename++) {
        ostringstream oss;
        oss << sync_dir << "/" << *filename;
        string filepath = oss.str();
        PLOGD << "File: " << filepath << endl;
        unique_ptr<FileHandler> file(new FileHandler(filepath));
        file->send(context->socket, context->connection->channel);
    }
}


void FileSync::loop() {
    this->sync_all();
    shared_ptr<Socket> socket = this->context->socket;
    int channel = this->context->connection->channel;
    shared_ptr<uint8_t> buffer((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t)));
    while(context->socket->get_message_sync(buffer.get(), context->connection->channel) != 0) {
        PLOGI << "Filesync waiting on channel " << channel << "..." << endl;
        sleep(1);
    }
}
