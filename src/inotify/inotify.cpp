#include "inotify.hpp"
using namespace std;

Inotify::Inotify(const char *folder_path){
    this->folder_path = folder_path;
    this->file_descriptor = inotify_init();
    if (this->file_descriptor == -1) {
        std::cerr << "Failed to initialize inotify" << std::endl;
        return;
    }
    
    this->watch_descriptor = inotify_add_watch(this->file_descriptor, this->folder_path, IN_MODIFY | IN_CREATE | IN_DELETE);
    if (this->watch_descriptor == -1) {
        std::cerr << "Failed to add watch" << std::endl;
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
        std::cerr << "Failed to read" << std::endl;
    }

    struct inotify_event* event = (struct inotify_event*) &(buffer)[0];
    if (event->len) {
        if (event->mask & IN_CREATE) {
            std::cout << "The file " << event->name << " was created." << std::endl;
        } else if (event->mask & IN_MODIFY) {
            std::cout << "The file " << event->name << " was modified." << std::endl;
        } else if (event->mask & IN_DELETE) {
            std::cout << "The file " << event->name << " was deleted." << std::endl;
        }
    }
}

void Inotify::closeInotify(){
    inotify_rm_watch(this->file_descriptor, this->watch_descriptor);
    close(this->file_descriptor);
}

