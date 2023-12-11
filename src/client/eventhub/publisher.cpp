#include "publisher.hpp"
#include <thread>

ClientPublisher::ClientPublisher(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    this->context = context;
    this->socket  = socket;
}
// aaaa
void ClientPublisher::loop() {
    const char *folder_path = this->context->sync_dir.c_str();
    PLOG_INFO << "Publisher folder path: " << folder_path << endl;
    shared_ptr<Inotify> inotify = make_shared<Inotify>(this->socket, folder_path);
    PLOG_INFO << "Publisher inotify: " << inotify << endl;
    int channel = this->socket->socket_fd;
    while (!*socket->interrupt) {
        PLOG_INFO << "Publisher channel: " << channel << endl;
        inotify->read_event();
        // colocar um sleep aqui
        std::this_thread::sleep_for(std::chrono::seconds(10));

        // inotify->read_event();
    }
};

void ClientPublisher::send_event() {
    PLOGE << "Publisher send event" << endl;
};
