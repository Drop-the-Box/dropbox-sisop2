#ifndef CLIENT_PUB_H
#define CLIENT_PUB_H

#include "../session/session.hpp"
#include "../file_io/inotify.hpp"
#include <memory>
#include <vector>

using namespace std;

class ClientPublisher {
    shared_ptr<ClientContext> context;
    bool *interrupt;
    Inotify *file_monitor;
    vector<std::string> commands = {
        "upload",
        "download",
        "delete",
        "list_server",
        "list_client",
        "exit",
        "help"
    };

public:
    ClientPublisher(shared_ptr<ClientContext> context, bool *interrupt);
    void loop();
    void send_event();
    void handle_upload(); 
    void handle_download();
    void handle_delete();
    void handle_list_server();
    void handle_list_client();
    string get_input_async();
    void handle_help();
};

#endif
