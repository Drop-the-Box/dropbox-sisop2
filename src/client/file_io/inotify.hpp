#ifndef INOTIFY_H
#define INOTIFY_H
// usar sempre snake case para nomear as variaveis
#include <plog/Log.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>
#include "../../common/file_io/file_io.hpp"

using namespace std;

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUFFER_LEN (1024 * (EVENT_SIZE + 16))

class Inotify {
    string             folder_path;
    int                file_descriptor;
    int                watch_descriptor;
    map<string, shared_ptr<FileMetadata>> files;
    shared_ptr<FileHandler> file_handler;

public:
    ConnectionManager *conn_manager;

    Inotify(ConnectionManager *conn_manager, const string folder_path);
    int  get_file_descriptor();
    int  get_watch_descriptor();
    vector<string> get_files();
    bool add_file(string filename);
    bool add_file(shared_ptr<FileMetadata> new_file);
    void delete_file(string filename);
    void read_event();
    void closeInotify();
};
#endif
