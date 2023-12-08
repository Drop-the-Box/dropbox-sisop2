#include "inotify.hpp"
using namespace std;

int main() {
    Inotify inotify("/home/teste");
    
    int file_descriptor = inotify.get_file_descriptor();
    cout << "file_descriptor: " << file_descriptor << endl;
    int watch_descriptor = inotify.get_watch_descriptor();
    cout << "watch_descriptor: " << watch_descriptor << endl;

    while (true) {
        inotify.read_event();
    }

    return 0;
}