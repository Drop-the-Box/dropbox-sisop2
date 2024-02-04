#ifndef VARS_H
#define VARS_H

#define MAX_REQUESTS 10
#define MAX_DEVICES_PER_USER 2
#define BUFFER_SIZE 1024
#define SERVER_PORT 6999
#define SOCKET_TIMEOUT 10
#include <string>

using namespace std;

typedef struct server_params {
    int pid;
    int port;
    string address;
} server_params;

#endif
