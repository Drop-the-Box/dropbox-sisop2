#ifndef INOTIFY_H
#define INOTIFY_H

#include "../../client/session/session.hpp"
#include "../../common/file_io/file_io.hpp"
#include "../../common/socket_io/socket.hpp"
#include <cstring>
#include <iostream>
#include <memory>
#include <plog/Log.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUFFER_LEN (1024 * (EVENT_SIZE + 16))

class Inotify {
    const char    *folder_path;
    int            file_descriptor;
    int            watch_descriptor;
    ClientContext *context;
    Socket        *socket;

public:
    Inotify(ClientContext *context, Socket *socket);
    int  get_file_descriptor();
    int  get_watch_descriptor();
    void read_event();
    void close_inotify();
};
#endif
