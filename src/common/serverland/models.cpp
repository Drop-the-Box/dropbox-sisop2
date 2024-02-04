#include "models.hpp"
#include "../vars.hpp"

ReplicaManager::ReplicaManager(int pid, bool is_leader) {
    this->server_pid = pid;
    this->is_leader = is_leader;
}

ReplicaManager::ReplicaManager(int pid, string address, int port, string base_dir, bool is_leader) {
    this->server_pid = pid;
    this->address = address;
    this->port = port;
    this->is_leader = is_leader;
    this->base_dir = base_dir;
}

shared_ptr<Socket> get_peer_socket(shared_ptr<ReplicaManager> server, bool *interrupt) {
    return make_shared<Socket>(
        server->address, server->port, interrupt, Client, BUFFER_SIZE
    );
}

