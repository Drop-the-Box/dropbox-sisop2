#include "publisher.hpp"
#include <cpp-inquirer/inquirer.h>
#include <iostream>
#include <map>
#include <plog/Log.h>
#include <thread>
#include <vector>
#include <pthread.h>
#include <sstream>
#include <regex>
#include <fstream>
#include <sys/sendfile.h>
#include <fcntl.h>



using namespace std;

void handle_upload(shared_ptr<alx::Inquirer> inquirer, const string sync_dir) {
    inquirer->add_question({"filepath", "Which file would you like to upload?"});
    inquirer->ask();
    const string file_path = inquirer->answer("filepath");
    PLOGI << "Uploading file to server from " << file_path << endl;
    regex  rgx("^(.*\\/)([^\\/]+)$");
    smatch matches;
    regex_search(file_path, matches, rgx);
    string filename = matches[2];
    ostringstream output_path;
    output_path << sync_dir << "/" << filename;
    ostringstream command;
    command << "cp " << file_path << " " << sync_dir << "/" << filename;
    PLOGW << command.str() << endl;
    try {
        system(command.str().c_str());
    } catch(const exception &exc) {
        PLOGE << "Cannot copy file" << file_path << ". Reason: " << exc.what() << endl; 
    }
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

void handle_list_server(shared_ptr<alx::Inquirer> inquirer){

};

void handle_list_client(shared_ptr<alx::Inquirer> inquirer){

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
    "exit"};
// for(const auto &[key, _]: command_map) { commands.push_back(key); };

void *run_file_monitor(void *monitor) {
    Inotify* file_monitor = (Inotify *)monitor;
    while(true) {
        file_monitor->read_event();
        usleep(1000);
    }
    return NULL;
}

ClientPublisher::ClientPublisher(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    this->context = context;
    this->socket  = socket;

    const char *folder_path = this->context->sync_dir.c_str();
    Inotify* inotify = new Inotify(this->socket, folder_path);

    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, run_file_monitor, inotify);
}


void ClientPublisher::loop() {

    while (!*socket->interrupt) {
        shared_ptr<alx::Inquirer> inquirer(new alx::Inquirer(alx::Inquirer("Drop the Box")));
        inquirer->add_question({"cmd", "Select a command:", commands});
        inquirer->ask();
        const string command = inquirer->answer("cmd");
        if (std::find(commands.begin(), commands.end(), command) != commands.end()) {
            PLOGW << "Command result: " << command << endl;
        }
        if (command.compare("upload") == 0) {
            handle_upload(inquirer, this->context->sync_dir.c_str());
        } else if (command.compare("download") == 0) {
            handle_download(inquirer);
        } else if (command.compare("delete") == 0) {
            handle_delete(inquirer);
        } else if (command.compare("list_server") == 0) {
            handle_list_server(inquirer);
        } else if (command.compare("list_client") == 0) {
            handle_list_client(inquirer);
        } else if (command.compare("exit") == 0) {
            PLOGI << "Bye!" << endl;
            exit(0);
        } else {
            PLOGE << "Command not found: " << command << endl;
        }
    }
    socket->close(socket->socket_fd);
    pthread_exit(NULL);
    return;
};

void ClientPublisher::send_event() {
    PLOGE << "Publisher send event" << endl;
};
