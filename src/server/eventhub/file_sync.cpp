#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>

#include "file_sync.hpp"
#include "../../common/vars.hpp"

using namespace std;

FileSync::FileSync(shared_ptr<ServerContext> context) {
    this->context = context;
}

void FileSync::sync_all() {
    cout << "-------------------- Connected to filesync" << endl;
    const string username = context->device->username;

    string sync_dir = FileHandler::get_sync_dir(username, DIR_SERVER);
    vector<string> files = FileHandler::list_files(sync_dir);

    vector<string>::iterator filename;
    for (filename=files.begin(); filename != files.end(); filename++) {
        ostringstream oss;
        oss << sync_dir << "/" << *filename;
        string filepath = oss.str();
        cout << "File: " << filepath << endl;
        FileHandler *file = new FileHandler(filepath);
        file->send(context->socket, context->connection->channel);
        file->close();
        free(file);
    }
}


void FileSync::loop() {
    this->sync_all();
    shared_ptr<Socket> socket = this->context->socket;
    int channel = this->context->connection->channel;
    shared_ptr<uint8_t> buffer((uint8_t *)malloc(BUFFER_SIZE));

        while(context->socket->get_message_async(buffer.get(), context->connection->channel) != 0) {
        cout << "Filesync waiting on channel " << channel << "..." << endl;
        sleep(1);
    }
}
