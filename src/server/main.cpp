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
#include <errno.h>
#include <sys/socket.h> ///< socket
#include <netinet/in.h> ///< sockaddr_in
#include <arpa/inet.h>  ///< getsockname

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

char *get_ip() {
    const char* google_dns_server = "8.8.8.8";
    int dns_port = 53;
    struct sockaddr_in serv;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    //Socket could not be created
    if(sock < 0)
    {
        std::cout << "Socket error" << std::endl;
    }
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(google_dns_server);
    serv.sin_port = htons(dns_port);
    int err = connect(sock, (const struct sockaddr*)&serv, sizeof(serv));
    if (err < 0)
    {
        std::cout << "Error number: " << errno
            << ". Error message: " << strerror(errno) << std::endl;
    }

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr*)&name, &namelen);

    // declara variavel para armazenar ip local da maquina
    char buffer[80];
    const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, 80);
    if(p == NULL)
    {
        std::cout << "Error number: " << errno
            << ". Error message: " << strerror(errno) << std::endl;
    }

    close(sock);
    // converte ip local para char*
    char* ip_local = new char[80];
    strcpy(ip_local, buffer);
    return ip_local;

}

int main(int argc, char *argv[]) {
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &consoleAppender);

    ::signal(SIGINT, stop_execution);
    int port         = SERVER_PORT;
    int max_requests = MAX_REQUESTS;
    int buffer_size  = BUFFER_SIZE;

    // char* socket_address = (char *)"0.0.0.0";
    // conseguir ip da maquina automaticamente
    char* ip_local = get_ip();
    PLOGI << "IP local: " << ip_local << endl;
    // convertendo char* para string
    //string socket_address = string(ip_local);
    string socket_address = "0.0.0.0";
    PLOGI << "Starting server on " << socket_address << ":" << port << endl;
    shared_ptr<Socket> socket = make_shared<Socket>(socket_address, port, &interrupt, Server, buffer_size, max_requests);

    unique_ptr<SessionManager> session_manager(new SessionManager(socket));
    session_manager->interrupt = &interrupt;
    session_manager->start();

    return 0;
}
