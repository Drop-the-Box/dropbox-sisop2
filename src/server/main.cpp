#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

// User-defined modules
#include "../common/socket_io/socket.hpp"
#include "../common/vars.hpp"
#include "session/session.hpp"

using namespace std;

bool interrupt = false;

void stop_execution(int signal) {
    PLOGW << "Caught signal: " << strsignal(signal) << "\n\n";
    interrupt = true;
}

int main(int argc, char *argv[]) {
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &consoleAppender);

    ::signal(SIGINT, stop_execution);
    int port         = SERVER_PORT;
    int max_requests = MAX_REQUESTS;
    int buffer_size  = BUFFER_SIZE;

    // char* socket_address = (char *)"0.0.0.0";
    string socket_address = "0.0.0.0";
    PLOGI << "Starting server on " << socket_address << ":" << port << endl;
    shared_ptr<Socket> socket = make_shared<Socket>(socket_address, port, &interrupt, Server, buffer_size, max_requests);

    unique_ptr<SessionManager> session_manager(new SessionManager(socket));
    session_manager->interrupt = &interrupt;
    session_manager->start();

    return 0;
}
