#include <iostream>
#include <fstream>
#include <cstring>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

// User-defined modules
#include "socket_io/socket.hpp"
#include "session/session.hpp"

#define MAX_REQUESTS 10
#define PORT 6999
#define BUFFER_SIZE 1024

using namespace std;

bool interrupt = false;

void stop_execution(int signal) {
    cout << "Caught signal: " << strsignal(signal) << "\n\n";
    interrupt = true;
}

int main(int argc, char *argv[]) {
    ::signal(SIGINT, stop_execution);
    int port = PORT;
    int max_requests = MAX_REQUESTS;
    int buffer_size = BUFFER_SIZE;

    char* socket_address = (char *)"0.0.0.0";
    cout << "Starting server on " << socket_address << ":" << port << endl;
    Socket *socket = new Socket(socket_address, port, buffer_size, max_requests);

    SessionManager *session_manager = new SessionManager(socket);
    session_manager->interrupt = &interrupt;
    session_manager->start();

    return 0;
}
