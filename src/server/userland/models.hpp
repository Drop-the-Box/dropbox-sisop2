#ifndef ULAND_MODELS_H
#define ULAND_MODELS_H

#include <semaphore.h> 
#include <vector>

#include "../session/models.hpp"
#include "../../common/socket_io/socket.hpp"

using namespace std;

class Session;

class Device {
    public:
        string address;
        string username;
        map<int, shared_ptr<Connection> > connections;

        Device(const string username, shared_ptr<Connection> connection);
};


class ServerContext {
    public:
        Device * device;
        shared_ptr<Connection> connection;
        shared_ptr<UserStore> storage;
        shared_ptr<Socket> socket;

        ServerContext(shared_ptr<Socket> socket, shared_ptr<Connection> connection, shared_ptr<UserStore>);
        void set_device(Device * device);
};


class UserStore {
    map<string, vector<shared_ptr<Session> > > users_sessions;
    map<string, map<string, Device * > > users_devices;
    sem_t devices_lock;
    sem_t files_lock;

    public:
        UserStore();
        ~UserStore();
        map<string, Device * > get_user_devices(const string username);
        vector<string> get_connected_users();
        void get_user_devices();
        bool add_user(const string username);
        bool register_device(const string username, shared_ptr<ServerContext> context);
        bool register_connection(const string username, shared_ptr<ServerContext> context);
        bool unregister_connection(shared_ptr<ServerContext> context);
        Device * get_device(const string username, const string address);
};

#endif
