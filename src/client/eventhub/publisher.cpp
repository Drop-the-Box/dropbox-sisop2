#include <cpp-inquirer/inquirer.h>
#include <plog/Log.h>
#include <vector>
#include <map>
#include <iostream>
#include "publisher.hpp"
#include <thread>

ClientPublisher::ClientPublisher(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    this->context = context;
    this->socket  = socket;
}


void handle_upload(shared_ptr<alx::Inquirer> inquirer) {
    inquirer->add_question({"file", "Which file would you like to upload?"});
    inquirer->ask();
    const string filename = inquirer->answer("file");
};


void handle_download(shared_ptr<alx::Inquirer> inquirer) {
    inquirer->add_question({"file", "Which file would you like to download?"});
    inquirer->ask();
    const string filename = inquirer->answer("file");
};


void handle_delete(shared_ptr<alx::Inquirer> inquirer) {
    inquirer->add_question({"file", "Which file would you like to delete?"});
    inquirer->ask();
    const string filename = inquirer->answer("file");
};


void handle_list_server(shared_ptr<alx::Inquirer> inquirer) {

};


void handle_list_client(shared_ptr<alx::Inquirer> inquirer) {

};

void handle_exit(shared_ptr<alx::Inquirer> inquirer) {
}

// using func=function<void(shared_ptr<alx::Inquirer>)>;
// 
// static const map<std::string, std::function<void>(*)(shared_ptr<alx::Inquirer>) > command_map = {
//     {"upload", &handle_upload },
//     {"download", &handle_download },
//     {"delete", &handle_delete },
//     {"list_server", &handle_list_server },
//     {"list_client", &handle_list_client },
//     {"exit", &handle_exit }
// };
vector<std::string> commands = {
    "upload", 
    "download",
    "delete", 
    "list_server",
    "list_client",
    "exit"
};
//for(const auto &[key, _]: command_map) { commands.push_back(key); };

void ClientPublisher::loop() {
<<<<<<< HEAD
    const char *folder_path = this->context->sync_dir.c_str();
    PLOG_INFO << "Publisher folder path: " << folder_path << endl;
    shared_ptr<Inotify> inotify = make_shared<Inotify>(this->socket, folder_path);
    PLOG_INFO << "Publisher inotify: " << inotify << endl;
    int channel = this->socket->socket_fd;
    while (!*socket->interrupt) {
        PLOG_INFO << "Publisher channel: " << channel << endl;
        inotify->read_event();
        // colocar um sleep aqui
        // std::this_thread::sleep_for(std::chrono::seconds(10));
=======
    shared_ptr<alx::Inquirer> inquirer(new alx::Inquirer(alx::Inquirer("Drop the Box")));

    while(!*socket->interrupt) {
        inquirer->add_question({"cmd", "Select a command:", commands});
        inquirer->ask();
        const string command = inquirer->answer("cmd");
        if (std::find(commands.begin(), commands.end(), command) != commands.end()) {
            PLOGW << "Command result: " << command << endl;
            if (command.compare("upload")) {
                handle_upload(inquirer);
            } else if (command.compare("download")) { 
                handle_download(inquirer);
            } else if (command.compare("delete")) {
                handle_delete(inquirer);
            } else if (command.compare("list_server")) {
                handle_list_server(inquirer);
            } else if (command.compare("list_client")) {
                handle_list_client(inquirer);
            } else {
                return;
            }
        } else {
            PLOGE << "Command not found: " << command << endl;
        }
>>>>>>> 2b4b8a4 (feat: add prompt)
    }
};

void ClientPublisher::send_event() {
    PLOGE << "Publisher send event" << endl;
};
