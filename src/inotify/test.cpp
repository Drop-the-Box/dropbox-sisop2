#include "inotify.hpp"
using namespace std;

int main() {
    Inotify inotify("/home/teste");
    
    int file_descriptor = inotify.get_file_descriptor();
    std::cout << "file_descriptor: " << file_descriptor << std::endl;
    int watch_descriptor = inotify.get_watch_descriptor();
    std::cout << "watch_descriptor: " << watch_descriptor << std::endl;

    while (true) {
        inotify.read_event();
    }

    return 0;
}