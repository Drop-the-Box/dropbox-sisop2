#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>

#include "file_sync.hpp"

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
        unique_ptr<FileHandler> file(new FileHandler(filepath));
        file->send(context->socket, context->connection->channel);
    }
}


void FileSync::loop() {
    this->sync_all();
    while(1) {
        sleep(1);
    }
}
