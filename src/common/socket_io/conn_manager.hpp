#ifndef CONN_MANAGER_HPP
#define CONN_MANAGER_HPP
#include "../../common/socket_io/socket.hpp"
#include "../../common/serverland/models.hpp"
#include "../../common/session/models.hpp"
#include <map>


class ConnectionResetError : public std::runtime_error {
    public:
        ConnectionResetError(string msg): runtime_error(msg.c_str()){};
};


class ConnectionManager {
    map<SessionType, shared_ptr<Socket>> socket_map;
    bool *interrupt;
    string username;
    pthread_mutex_t reconnect_mutex;
    bool reconnecting = false;

    bool setup_connection(shared_ptr<Socket> socket, SessionType kind);
    void init_connections();
    shared_ptr<ReplicaManager> get_server_leader();
    void check_socket(SessionType kind);
    bool probe_server(shared_ptr<Socket> socket);
    void reconnect();
    ~ConnectionManager();
    
    public:
        ConnectionManager(string username, bool *interrupt);
        shared_ptr<Socket> get_socket(SessionType kind);
        int send(shared_ptr<Packet> packet, SessionType kind);
        int get_message(uint8_t *buffer, SessionType kind);
        void stop_connections();
};
#endif
