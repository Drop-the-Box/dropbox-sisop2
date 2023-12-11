#ifndef SESSION_SRV_MODELS_H
#define SESSION_SRV_MODELS_H

#include "../../common/session/models.hpp"
#include <arpa/inet.h>
#include <memory>
#include <netinet/in.h>
#include <string>

using namespace std;

class UserStore;

class Connection {
public:
    SessionType session_type;
    string      address;
    int         port;
    pthread_t  *thread_id;
    int         channel;

    Connection(char *address, int port, int channel);
    void   set_thread_id(pthread_t *thread_id);
    string get_full_address();
    void   set_session_type(SessionType session_type);
};

#endif
