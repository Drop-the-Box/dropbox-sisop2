#include "testInotify.hpp"

#include <iostream>
#include <sys/inotify.h>
#include <unistd.h>
#include <memory>

using namespace std;

// usar snake case

Inotify::Inotify(std::string folderPath){
    this->fileDescriptor = initializeInotify();
    this->watchDescriptor = addWatch(folderPath);
    this->bufferSize = sizeof(struct inotify_event) + NAME_MAX + 1;
    this->buffer = unique_ptr<char *>(new char[this->bufferSize]);
}

int Inotify::initializeInotify() {
    int fd = inotify_init();
    if (fd == -1) {
        std::cerr << "Failed to initialize inotify" << std::endl;
        return -1;
    }
    return fd;
}

int Inotify::addWatch(std::string folderPath) {
    int wd = inotify_add_watch(this->fileDescriptor, folderPath.c_str(), IN_MODIFY | IN_DELETE);
    if (wd == -1) {
        std::cerr << "Failed to add watch for folder: " << folderPath << std::endl;
        close(this->fileDescriptor);
        return -1;
    }
    return wd;
}

void Inotify::closeInotify() {
    inotify_rm_watch(this->fileDescriptor, this->watchDescriptor);
    close(this->fileDescriptor);
}

int Inotify::readInotifyEvents() {
    ssize_t bytesRead = read(this->fileDescriptor, this->buffer, sizeof(this->buffer));
    if (bytesRead == -1) {
        std::cerr << "Failed to read inotify events" << std::endl;
        return -1;
    }
    return bytesRead;
}

void Inotify::processInotifyEvents(int bytesRead) {
    for (char* ptr = this->buffer; ptr < this->buffer + bytesRead;) {
        struct inotify_event* event = reinterpret_cast<struct inotify_event*>(ptr);
        std::string fileName = event->name;
        std::string eventType;

        // Determine the type of event
        if (event->mask & IN_MODIFY) {
            eventType = "modified";
            std::cout << "File " << fileName << " was modified" << std::endl;

        } else if (event->mask & IN_DELETE) {
            eventType = "deleted";
            std::cout << "File " << fileName << " was deleted" << std::endl;
        }
        else {
            std::cout << "Unknown event - " << event->mask << std::endl;
        }

        // Print the file name and event type
        std::cout << "File " << fileName << " was " << eventType << std::endl;

        ptr += sizeof(struct inotify_event) + event->len;
    }
}

