#ifndef SESSION_H
#define SESSION_H

#include "../socket_io/socket.hpp"

#define MAX_REQUESTS 10


class SessionManager {
    int num_threads;
    int channels[MAX_REQUESTS];
    Socket *socket;
    pthread_t thread_pool[MAX_REQUESTS];

    static void *handle_session(void *args);
    void create_session(int channel);

    public:
        bool *interrupt;

        SessionManager(Socket *socket);
        void start();
        void stop(int signal);
};


class Session {
    pthread_t *thread;
    char *buffer;


    public:
        int channel;
        Socket *socket;

        Session(Socket *socket, int channel, pthread_t *thread);
        int get_message();
        int stream_messages();
};
#endif
