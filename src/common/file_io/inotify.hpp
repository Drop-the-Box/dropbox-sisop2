#ifndef INOTIFY_H
#define INOTIFY_H
// usar sempre snake case para nomear as variaveis
#include <memory>
#include <sys/inotify.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <memory>
#include <cstring>
#include "../session/session.hpp"
#include "../../common/socket_io/socket.hpp"
#include "../../common/file_io/file_io.hpp"
#include <plog/Log.h>



using namespace std;

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUFFER_LEN (1024*(EVENT_SIZE+16))

class Inotify{
    const char *folder_path;
    int file_descriptor;
    int watch_descriptor;
    ClientContext *context;
    Socket *socket;
    public:
        Inotify(ClientContext *context, Socket *socket);
        int get_file_descriptor();
        int get_watch_descriptor();
        void read_event();
        void closeInotify();
    };
#endif
