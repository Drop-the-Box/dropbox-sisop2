#ifndef REPL_MODELS_HPP
#define REPL_MODELS_HPP

#include <string>
#include "../socket_io/socket.hpp"

using namespace std;

class ReplicaManager {
public:
    string address;
    int port;
    int server_pid;
    bool is_leader;

    ReplicaManager(int pid, bool is_leader = false);
    ReplicaManager(int pid, string address, int port);
};

shared_ptr<Socket> get_peer_socket(shared_ptr<ReplicaManager> server, bool *interrupt);

#endif
