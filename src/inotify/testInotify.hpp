#ifndef INOTIFY_H
#define INOTIFY_H

#include <memory>

#include "testInotify.cpp"

using namespace std;

class Inotify {
    int fileDescriptor;
    int watchDescriptor;
    int bufferSize;
    unique_ptr<char *> buffer; // Using smart pointer

public:
    Inotify(std::string folderPath);
    int initializeInotify();
    int addWatch(std::string folderPath);
    void closeInotify();
    int readInotifyEvents();
    void processInotifyEvents(int bytesRead);
};

#endif // INOTIFY_H