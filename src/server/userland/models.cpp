#include <cstring>
#include <fstream>
#include <iostream>
#include <plog/Log.h>
#include <regex>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>

#include "../session/session.hpp"
#include "models.hpp"

using namespace std;

#define USER_MAX_DEVICES 2

Device::Device(const string username, shared_ptr<Connection> connection) {
    this->username                      = username;
    this->address                       = connection->address;
    this->connections[connection->port] = connection;
}

UserStore::UserStore() {
    sem_init(&this->devices_lock, 0, 1);
};

UserStore::~UserStore() {
    sem_destroy(&this->devices_lock);
}

bool UserStore::add_user(const string username) {
    bool user_added = false;
    if (this->users_devices.find(username) == this->users_devices.end()) {
        map<string, Device *> device_map;
        this->users_devices[username] = device_map;
        user_added                    = true;
    } else {
        PLOGW << "User " << username << " already registered." << endl;
    }
    return user_added;
}

bool UserStore::register_device(const string username, shared_ptr<ServerContext> context) {
    map<string, Device *> device_map;

    shared_ptr<Connection> connection = context->connection;
    try {
        device_map = this->users_devices.at(username);
    } catch (out_of_range) {
        PLOGW << "User " << username << " is not registered. Registering..." << endl;
        if (!this->add_user(username)) {
            return false;
        }
        device_map = this->users_devices.at(username);
    }

    //  This is where we limit max devices online
    if (device_map.size() == USER_MAX_DEVICES) {
        PLOGW << "User " << username << " reached maximum of simultaneously connected devices: " << USER_MAX_DEVICES << endl;
        PLOGW << "Connection from device with address " << connection->address << " rejected." << endl;
        return false;
    }
    Device *device;
    if (device_map.find(connection->address) == device_map.end()) {
        device                                             = new Device(username, connection);
        this->users_devices[username][connection->address] = device;
    } else {
        PLOGW << "Device with address " << connection->address << " already registered" << endl;
        device = this->users_devices.at(username).at(connection->address);
    }
    context->set_device(device);

    return true;
}

bool UserStore::register_connection(const string username, shared_ptr<ServerContext> context) {
    sem_wait(&this->devices_lock);

    map<string, Device *> device_map;
    bool                  result = true;

    try {
        device_map = this->users_devices.at(username);
    } catch (out_of_range) {
        if (!this->add_user(username))
            return false;
        device_map = this->users_devices.at(username);
    }
    shared_ptr<Connection> connection = context->connection;

    if (device_map.find(connection->address) == device_map.end()) {
        sem_post(&this->devices_lock);
        result = this->register_device(username, context);
    } else {
        Device *device                        = device_map[connection->address];
        device->connections[connection->port] = connection;
        context->set_device(device);
    }

    sem_post(&this->devices_lock);
    return result;
}

bool UserStore::unregister_connection(shared_ptr<ServerContext> context) {
    if (context->device != NULL) {
        sem_wait(&this->devices_lock); // acquiring lock
                                       //
        shared_ptr<Connection> connection = context->connection;
        auto iter = context->device->connections.find(connection->port);
        if (iter != context->device->connections.end()) {
            context->device->connections.erase(iter);
        }
        if (context->device->connections.size() == 0) {
            string username   = context->device->username;
            auto device_key = this->users_devices[username].find(context->device->address);
            if (device_key != this->users_devices[username].end()) {
                PLOGI << "Unregistering device " << context->device->address << " from user " << username << endl;
                this->users_devices[username].erase(device_key);
            }
            if (this->users_devices[username].size() == 0) {
                map<string, map<string, Device *>>::iterator user_key = this->users_devices.find(username);
                this->users_devices.erase(user_key);
            }
        }

        sem_post(&this->devices_lock);
    }
    return true;
}

map<string, Device *> UserStore::get_user_devices(const string username) {
    try {
        return this->users_devices.at(username);
    } catch (out_of_range) {
        PLOGE << "Error! Cannot find username " << username << endl;
        throw;
    }
}

Device *UserStore::get_device(const string username, const string address) {
    try {
        return this->users_devices.at(username).at(address);
    } catch (out_of_range) {
        PLOGE << "Cannot find device at address " << address << " from user " << username << endl;
    }
    return NULL;
}

// vector<int> UserStore::get_connected_channels(int current_server) {
// 
// }

vector<string> UserStore::get_connected_users() {
    vector<string>                           users;
    map<string, vector<shared_ptr<Session>>> sessions = this->users_sessions;
    transform(
        sessions.begin(), sessions.end(), back_inserter(users),
        [](pair<string, vector<shared_ptr<Session>>> p) { return p.first; } // NOLINT
    );
    return users;
}

void UserStore::get_user_devices() {
}

vector<int> UserStore::get_all_channels() {
    auto users = this->users_devices;
    vector<int> all_connections;
    for (auto it = users.begin(); it != users.end(); it++) {
        for (auto dit=it->second.begin(); dit != it->second.end(); dit++) {
            auto connections = dit->second->connections;
            for (auto cit=connections.begin(); cit != connections.end(); cit++) {
                all_connections.push_back(cit->second->channel);
            }
        }
    }
    return all_connections;
}

ServerContext::ServerContext(
    shared_ptr<Socket> socket,
    shared_ptr<Connection> connection,
    shared_ptr<UserStore> storage,
    ServerElectionService *election_service
) {
    this->socket     = socket;
    this->connection = connection;
    this->storage    = storage;
    this->device     = NULL;
    this->election_service = election_service;
    this->repl_service = election_service->repl_service;
}

void ServerContext::set_device(Device *device) {
    this->device = device;
}
