#ifndef MODELS_H
#define MODELS_H

#include "../session/session.hpp"


class Device {
    public:
        char *address;
        string username;
        map<int, Connection *> connections;

        Device(char *username, Connection *connection);
};

class UserStore {
    map<string, vector<Session *> > users_sessions;
    map<string, map<string, Device * > > users_devices;

    public:
        vector<Device *> get_user_devices(char *username);
        vector<string> get_connected_users();
        void get_user_devices();
        bool add_user(char *username);
        bool register_device(char *username, Connection *connection);
        bool register_connection(char *username, Connection *connection);
        bool unregister_connection(Connection *connection);
        Device * get_device(char *username, string address);
};

#endif
