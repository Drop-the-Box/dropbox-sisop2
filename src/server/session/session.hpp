#ifndef SESSION_H
#define SESSION_H

#include <vector>
#include <map>
#include <string>
#include <string.h>
#include <memory>

#include "../socket_io/socket.hpp"
#include "models.hpp"

using namespace std;

#define MAX_REQUESTS 10

class SessionManager {
    int num_threads;
    int channels[MAX_REQUESTS];
    shared_ptr<Socket> socket;
    pthread_t thread_pool[MAX_REQUESTS];

    static void *handle_session(void *args);
    void create_session(int channel, Connection *connection, UserStore *storage);

    public:
        bool *interrupt;

        SessionManager(shared_ptr<Socket> socket);
        void start();
        void stop(int signal);
};

class Session {
    uint8_t *buffer;
    UserStore *storage;


    public:
        int channel;
        shared_ptr<Socket> socket;
        Device *device;
        SessionType type;
        Connection *connection;

        Session(shared_ptr<Socket> socket, int channel, Connection *connection, UserStore *storage);
        int get_message_sync(uint8_t *buffer);
        int get_message_async(uint8_t *buffer);
        int loop();
        bool setup();
        void run();
        void teardown();
};


#endif
