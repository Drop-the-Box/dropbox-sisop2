#include <iostream>
#include <fstream>
#include <cstring>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <memory>

// User-defined modules
#include "../common/vars.hpp"
#include "../common/socket_io/socket.hpp"
#include "session/session.hpp"

using namespace std;

bool interrupt = false;

void stop_execution(int signal) {
    cout << "Caught signal: " << strsignal(signal) << "\n\n";
    interrupt = true;
}

int main(int argc, char *argv[]) {
    ::signal(SIGINT, stop_execution);
    int port = SERVER_PORT;
    int max_requests = MAX_REQUESTS;
    int buffer_size = BUFFER_SIZE;

    // char* socket_address = (char *)"0.0.0.0";
    string socket_address = "0.0.0.0";
    cout << "Starting server on " << socket_address << ":" << port << endl;
    shared_ptr<Socket> socket(new Socket(socket_address, port, Server, buffer_size, max_requests));

    unique_ptr<SessionManager> session_manager(new SessionManager(socket));
    session_manager->interrupt = &interrupt;
    session_manager->start();

    return 0;
}
