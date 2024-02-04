#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../common/socket_io/socket.hpp"
#include "../server/session/session.hpp"
#include "session/session.hpp"

#define PORT 6999
#define ADDRESS "dtb-server"

using namespace std;

int main(int argc, char *argv[]) {

    char *username_ptr = argv[1];
    // char *address_ptr  = argv[2];
    // char *port_ptr     = argv[3];

    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::info, &consoleAppender);

    if (username_ptr == NULL) {
        PLOGE << "Error! No username provided. Exiting..." << endl;
        exit(1);
    }
    // if (address_ptr == NULL) {
    //     PLOGE << "Error! No server address provided. Exiting..." << endl;
    //     exit(1);
    // }
    // if (port_ptr == NULL) {
    //     PLOGE << "Error! No server port provided. Exiting..." << endl;
    //     exit(1);
    // }
    // int                              server_port = (int)atoi(argv[3]);
    // string                           server_addr(address_ptr);
    string                           username(username_ptr);
    unique_ptr<ClientSessionManager> manager(new ClientSessionManager(username));
    return 0;
}
