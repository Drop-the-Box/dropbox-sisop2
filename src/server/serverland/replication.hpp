#ifndef REPLICATION_HPP
#define REPLICATION_HPP

#include <plog/Log.h>
#include <map>
#include <memory>
#include "../../common/serverland/models.hpp"

using namespace std;

class ServerStore {
public:
    map<int, shared_ptr<ReplicaManager>> replica_managers;

    bool has_server(int pid);
    void add_server(int pid, shared_ptr<ReplicaManager> server);
    void set_leader(int pid);
};


class ServerElectionService {
public:
    shared_ptr<ServerStore> server_store;
    shared_ptr<ReplicaManager> current_server;
    bool *interrupt;

    void sync_servers();
    void start_election();
    void notify_elected();
    void update_leader(int leader_pid);

    ServerElectionService(
        shared_ptr<ReplicaManager> current_server, shared_ptr<ServerStore> server_store, bool *interrupt_ptr
    );
};

#endif
