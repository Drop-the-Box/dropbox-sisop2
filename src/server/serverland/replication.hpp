#ifndef REPLICATION_HPP
#define REPLICATION_HPP

#include <plog/Log.h>
#include <map>
#include <memory>
#include "../../common/eventhub/models.hpp"
#include "../../common/serverland/models.hpp"

using namespace std;

class ServerStore {
public:
    map<int, shared_ptr<ReplicaManager>> replica_managers;
    map<int, shared_ptr<ReplicaManager>> all_servers;
    shared_ptr<ReplicaManager> current_server;

    ServerStore(shared_ptr<ReplicaManager> current_server);

    bool has_server(int pid);
    void add_server(int pid, shared_ptr<ReplicaManager> server);
    void set_leader(int pid);
    void remove_server(int pid);
};


class ReplicationService {
    bool *interrupt;
    pthread_mutex_t backup_mutex;
    map<int, shared_ptr<Socket>> server_socket_map; 

    public:
        shared_ptr<ServerStore> server_store;

        ReplicationService(shared_ptr<ServerStore> storage, bool *interrupt_ptr);
        void connect_backups();
        void connect_backup(shared_ptr<ReplicaManager> server_rm);
        void disconnect_backups();
        void send_file(string username, string file);
        void send_command(string username, shared_ptr<Command> command);
        bool has_backup(int pid);
};

class ServerElectionService {
public:
    shared_ptr<ReplicationService> repl_service;
    shared_ptr<ServerStore> server_store;
    shared_ptr<ReplicaManager> current_server;
    bool *interrupt;
    pthread_mutex_t election_mutex;
    pthread_t leader_pooling_tid;

    void sync_servers();
    void start_election();
    bool notify_elected();
    void update_leader(int leader_pid);
    string get_status();

    ServerElectionService(
        shared_ptr<ReplicationService> repl_service, bool *interrupt_ptr
    );
    ~ServerElectionService();
    bool probe_server(shared_ptr<ReplicaManager> server_rm);
    static void *check_leader(void *svc);
    bool notify_elected_to_backup(shared_ptr<ReplicaManager> server_rm);
};

#endif
