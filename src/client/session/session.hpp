#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H
#include <memory>

#include "../../common/session/models.hpp"
#include "../../common/socket_io/socket.hpp"

using namespace std;

class ClientContext {
public:
    string      server_addr;
    int         server_port;
    string      username;
    SessionType session_type;
    string      sync_dir;

    ClientContext(
        string server_addr, int server_port, string username,
        string sync_dir, SessionType session_type);

    inline string get_folder_path() { return this->sync_dir; }
    //  inline int    get_channel() { return this->channel; }
};

class ClientSessionManager {
public:
    ClientSessionManager(string address, int port, string username);
    static void *handle_session(void *request);
};

class ClientSession {
    shared_ptr<Socket>         socket;
    shared_ptr<SessionRequest> request;
    shared_ptr<ClientContext>  context;

public:
    ClientSession(shared_ptr<ClientContext> context, shared_ptr<Socket> socket);
    bool setup();
    void run();
};

#endif
