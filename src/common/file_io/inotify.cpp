#include "inotify.hpp"
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

Inotify::Inotify(ClientContext *context, Socket *socket) {
    this->folder_path     = context->get_folder_path().c_str();
    this->file_descriptor = inotify_init();
    this->context         = context;
    this->socket          = socket;
    if (this->file_descriptor == -1) {
        PLOGE << "Failed to initialize inotify" << endl;
        return;
    }

    this->watch_descriptor = inotify_add_watch(this->file_descriptor, this->folder_path, IN_MODIFY | IN_CREATE | IN_DELETE);
    if (this->watch_descriptor == -1) {
        PLOGE << "Failed to add watch" << endl;
        return;
    }
}

int Inotify::get_file_descriptor() {
    return this->file_descriptor;
}

int Inotify::get_watch_descriptor() {
    return this->watch_descriptor;
}

void Inotify::read_event() {
    char buffer[BUFFER_LEN];

    int length = read(this->file_descriptor, buffer, BUFFER_LEN);
    if (length < 0) {
        PLOGE << "Failed to read" << endl;
    }

    struct inotify_event *event = (struct inotify_event *)&(buffer)[0];

    string full_file_path;

    if (event->len) {

        if (event->mask & IN_CREATE) {
            PLOGE << "The file " << event->name << " was created." << endl;
            full_file_path = this->folder_path + string(event->name);
            FileHandler fileHandler(full_file_path);
            fileHandler.send(this->socket, this->socket->socket_fd);
        } else if (event->mask & IN_MODIFY) {
            PLOGE << "The file " << event->name << " was modified." << endl;
            full_file_path = this->folder_path + string(event->name);
            FileHandler fileHandler(full_file_path);
            fileHandler.send(this->socket, this->socket->socket_fd);
        } else if (event->mask & IN_DELETE) {
            PLOGE << "The file " << event->name << " was deleted." << endl;
            full_file_path = this->folder_path + string(event->name);
            FileHandler fileHandler(full_file_path);
            // ver função de deletar arquivo no servidor
        }
    }
}

void Inotify::close_inotify() {
    inotify_rm_watch(this->file_descriptor, this->watch_descriptor);
    close(this->file_descriptor);
}
