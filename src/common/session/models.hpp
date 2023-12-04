#ifndef SESSION_CMN_MODELS_H
#define SESSION_CMN_MODELS_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <map>

using namespace std;

enum SessionType {
    Unknown, FileExchange, CommandPublisher, CommandSubscriber
};

static const map<SessionType, string> session_type_map = {
    { CommandPublisher, "subscriber" },
    { CommandSubscriber, "publisher" },
    { FileExchange, "file_exchange" }
};

class SessionRequest {
    public:
        SessionType type;
        string username;
        uint16_t uname_s;

        SessionRequest(uint8_t *bytes);
        SessionRequest(SessionType type, string username);
        size_t to_bytes(uint8_t** bytes_ptr);
};


#endif
