#ifndef SESSION_H
#define SESSION_H

#include <map>
#include <memory>
#include <string.h>
#include <string>
#include <vector>

#include "../../common/session/models.hpp"
#include "../../common/socket_io/socket.hpp"
#include "../userland/models.hpp"
#include "../serverland/replication.hpp"
#include "../../common/vars.hpp"

using namespace std;

class SessionManager {
    int num_threads;
    int channels[MAX_REQUESTS];
    shared_ptr<Socket>  socket;
    shared_ptr<ReplicaManager> current_server;
    shared_ptr<ServerStore> server_storage;

    map<int, pthread_t> thread_pool;

    static void *handle_session(void *args);
    void create_session(int channel, shared_ptr<ServerContext> context);
    void sync_servers();

public:
    bool *interrupt;
    ServerElectionService *election_svc;

    SessionManager(shared_ptr<Socket> socket, ServerElectionService *election_svc);
    void start(int pid);
    void stop(int signal);
};

class Session {
public:
    shared_ptr<ServerContext> context;
    int                       channel;
    SessionType               type;

    Session(shared_ptr<ServerContext>);
    bool setup();
    bool setup_client(shared_ptr<Packet> packet);
    bool setup_server(shared_ptr<Packet> packet);
    void handle_server_probe(shared_ptr<Packet> packet);
    void handle_server_election(shared_ptr<Packet> packet);
    void handle_server_elected(shared_ptr<Packet> packet);
    void run();
    void teardown();
};

#endif
