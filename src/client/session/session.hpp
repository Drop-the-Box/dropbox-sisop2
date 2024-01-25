#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H
#include <memory>

#include "../../common/session/models.hpp"
#include "../../common/socket_io/socket.hpp"
#include "models.hpp"

using namespace std;

class ClientSessionManager {
public:
    ClientSessionManager(string username);
    static void *handle_session(void *request);
};

class ClientSession {
    shared_ptr<Socket>         socket;
    shared_ptr<SessionRequest> request;
    shared_ptr<ClientContext>  context;

public:
    ClientSession(shared_ptr<ClientContext> context);
    bool setup();
    void run();
};

#endif
