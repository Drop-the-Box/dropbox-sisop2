#include "inotify.hpp"

Inotify::Inotify(shared_ptr<Socket> socket, const char *folder_path) {
    this->folder_path     = folder_path;
    this->file_descriptor = inotify_init();
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
    PLOG_INFO << "Inotify read event" << endl;

    char   buffer[BUFFER_LEN];
    string full_file_path;
    int    length = read(this->file_descriptor, buffer, BUFFER_LEN);
    if (length < 0) {
        PLOGE << "Failed to read" << endl;
    }

    struct inotify_event *event = (struct inotify_event *)&(buffer)[0];
    if (event->len) {
        if (event->mask & IN_CREATE) {
            full_file_path = string(this->folder_path) + "/" + string(event->name);

            PLOGI << "The file " << full_file_path << " was created." << endl;
            FileHandler fileHandler(full_file_path);
            if (fileHandler.send(this->socket, this->socket->socket_fd)) {
                PLOGI << "File " << full_file_path << " sent successfully." << endl;
            } else {
                PLOGE << "Error sending file " << full_file_path << "." << endl;
            }
        } else if (event->mask & IN_MODIFY) {
            full_file_path = string(this->folder_path) + "/" + string(event->name);
            PLOGI << "The file " << full_file_path << " was modified." << endl;
            FileHandler fileHandler(full_file_path);
            if (fileHandler.send(this->socket, this->socket->socket_fd)) {
                PLOGI << "File " << full_file_path << " sent successfully." << endl;
            } else {
                PLOGE << "Error sending file " << full_file_path << "." << endl;
            }
        } else if (event->mask & IN_DELETE) {
            full_file_path = string(this->folder_path) + "/" + string(event->name);
            PLOGI << "The file " << full_file_path << " was deleted." << endl;
            FileHandler fileHandler(full_file_path);
            // ver função de deletar arquivo no servidor
        }
    }
    PLOG_INFO << "Inotify read event end" << endl;
}

void Inotify::closeInotify() {
    inotify_rm_watch(this->file_descriptor, this->watch_descriptor);
    close(this->file_descriptor);
}
