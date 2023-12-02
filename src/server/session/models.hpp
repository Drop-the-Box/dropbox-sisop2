#ifndef SESSION_MODELS_H
#define SESSION_MODELS_H

#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
using namespace std;

class Device;
class UserStore;

enum SessionType {
    Unknown, FileExchange, CommandPublisher, CommandSubscriber
};


class Connection {
    public:
        SessionType session_type;
        char* address;
        int port;
        pthread_t *thread_id;
        int channel;
        Device *device;

        Connection(char *address, int port, int channel);
        ~Connection(void);
        void set_thread_id(pthread_t *thread_id);
        string get_full_address();
        void set_device(Device *device);
        void set_session_type(SessionType session_type);
};

class SessionRequest {
    public:
        SessionType type;
        char *username;
        uint16_t uname_s;

        SessionRequest(uint8_t *bytes);
        SessionRequest(SessionType type, char *username);
        ~SessionRequest(void);
        size_t to_bytes(uint8_t** bytes_ptr);
};

#endif
