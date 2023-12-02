#include "testinotify.hpp"


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <folder path>" << std::endl;
        return 1;
    }

    Inotify inotify(argv[1]);
    int fileDescriptor = inotify.initializeInotify();
    int watchDescriptor = inotify.addWatch(argv[1]);
    int bufferSize = sizeof(struct inotify_event) + NAME_MAX + 1;
    char buffer[bufferSize];

    while (true) {
        ssize_t bytesRead = read(fileDescriptor, buffer, sizeof(buffer));
        if (bytesRead == -1) {
            std::cerr << "Failed to read inotify events" << std::endl;
            return 1;
        }

        for (char* ptr = buffer; ptr < buffer + bytesRead;) {
            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(ptr);
            std::string fileName = event->name;
            std::string eventType;

            // Determine the type of event
            if (event->mask & IN_MODIFY) {
                eventType = "modified";
            } else if (event->mask & IN_DELETE) {
                eventType = "deleted";
            }

            std::cout << "File " << fileName << " was " << eventType << std::endl;

            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    inotify.closeInotify();
    return 0;
}