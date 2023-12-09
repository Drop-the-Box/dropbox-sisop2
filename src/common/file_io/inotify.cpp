#include "inotify.hpp"

Inotify::Inotify(const char *folder_path){
    this->folder_path = folder_path;
    this->file_descriptor = inotify_init();
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

int Inotify::get_file_descriptor(){
    return this->file_descriptor;
}

int Inotify::get_watch_descriptor(){
    return this->watch_descriptor;
}

void Inotify::read_event(){
    char buffer[BUFFER_LEN];

    int length = read(this->file_descriptor, buffer, BUFFER_LEN);
    if (length < 0) {
        PLOGE << "Failed to read" << endl;
    }

    struct inotify_event* event = (struct inotify_event*) &(buffer)[0];
    if (event->len) {
        if (event->mask & IN_CREATE) {
            cout << "The file " << event->name << " was created." << endl;
        } else if (event->mask & IN_MODIFY) {
            cout << "The file " << event->name << " was modified." << endl;
        } else if (event->mask & IN_DELETE) {
            cout << "The file " << event->name << " was deleted." << endl;
        }
    }
}

void Inotify::closeInotify(){
    inotify_rm_watch(this->file_descriptor, this->watch_descriptor);
    close(this->file_descriptor);
}
