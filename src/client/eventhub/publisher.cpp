#include "publisher.hpp"
#include <iostream>
#include <plog/Log.h>
#include <vector>
#include <pthread.h>
#include <sstream>
#include <regex>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <future>
#include <chrono>
#include <iostream>



<<<<<<< Updated upstream
#include "../file_io/inotify.hpp"
=======
#include "../../common/vars.hpp"
>>>>>>> Stashed changes



using namespace std;

void ClientPublisher::handle_upload() {
    string sync_dir = this->context->sync_dir;
    cout << "Which file would you like to upload?  ";
    const string file_path = this->get_input_async();

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

void ClientPublisher::handle_download() {
    cout << "\nWhich file would you like to download?  ";
    const string filename = this->get_input_async();
};

void ClientPublisher::handle_delete() {
    cout << "\nWhich file would you like to delete?  ";
    const string filename = this->get_input_async();
    shared_ptr<Command> del_command(new Command(DeleteFile, filename));
    context->conn_manager->send_command(del_command, CommandPublisher);
};

void ClientPublisher::handle_list_server(){

};

void ClientPublisher::handle_list_client(){
    vector<string> files = this->file_monitor->get_files();
    cout << "Files stored in this client: \n" << endl;
    cout << "----------------------------------------------" << endl;
    for(auto it = files.begin(); it != files.end(); it++) {
        cout << "\t" << *it << endl;
    }
    cout << "----------------------------------------------" << endl;
};

void ClientPublisher::handle_help() {
    cout << "Available commands:" << endl;
    cout << "\tupload\t\t - Upload a file to the server." << endl;
    cout << "\tdownload\t - Download a file from the server." << endl;
    cout << "\tdelete\t\t - Delete a file from the local and server." << endl;
    cout << "\tlist_server\t - List files on the server." << endl;
    cout << "\tlist_client\t - List files on sync directory." << endl;
    cout << "\texit\t\t - Ends the sync session and this program." << endl;
    cout << "\thelp\t\t - Shows this help list." << endl;
}


void *run_file_monitor(void *ctx) {
    ClientContext *context = (ClientContext *)(ctx);
    PLOGI << "Waiting for get_sync_dir to finish to start inotify" << endl;
    while(!*context->is_listening_dir);
    PLOGI << "Starting inotify..." << endl;
    while(!*context->interrupt) {
        context->file_monitor->read_event();
        usleep(1000);
    }
    return NULL;
}

ClientPublisher::ClientPublisher(shared_ptr<ClientContext> context, bool *interrupt) {
    this->context = context;
    this->interrupt = interrupt;

    const string folder_path = this->context->sync_dir;

    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, run_file_monitor, context.get());
}

string ClientPublisher::get_input_async() {
    char in_char;
    string input = "";

    while(cin.get(in_char) && !*interrupt && !context->conn_manager->has_error(CommandPublisher)) {
        if (in_char == '\n' && input.length() != 0) {
            PLOGI << "Got command: `" << input << "`" << endl;
            return input;
        } else {  // otherwise add it to the string so far
            input.append(1, in_char);
        }
    }
    if (*interrupt) {
        throw UserInterruptError("Input interrupted by user.");
    }
    return input;
}

void ClientPublisher::loop() {

    string command;
    ConnectionManager *conn_manager = context->conn_manager;
    while (!*this->interrupt && !conn_manager->has_error(CommandPublisher)) {
        cout << "Type a command (help to list available ones): ";
        try {
            command = this->get_input_async();
        } catch (UserInterruptError *exc) {
            break;
        }
        if (std::find(commands.begin(), commands.end(), command) != commands.end()) {
            PLOGW << "Command result: " << command << endl;
        }
        if (command.compare("upload") == 0) {
            handle_upload();
        } else if (command.compare("download") == 0) {
            handle_download();
        } else if (command.compare("delete") == 0) {
            handle_delete();
        } else if (command.compare("list_server") == 0) {
            handle_list_server();
        } else if (command.compare("list_client") == 0) {
            handle_list_client();
        } else if (command.compare("exit") == 0) {
            PLOGI << "Bye!" << endl;
            exit(0);
        } else if (command.compare("help") == 0) {
            handle_help();
        } else {
            PLOGE << "Command not found: " << command << endl;
        }
    }
    pthread_exit(NULL);
    return;
};

void ClientPublisher::send_event() {
    PLOGE << "Publisher send event" << endl;
};
