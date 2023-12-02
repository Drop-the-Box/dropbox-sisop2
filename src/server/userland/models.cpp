#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstring>
#include <regex>

#include "models.hpp"
#include "../session/session.hpp"

using namespace std;


#define USER_MAX_DEVICES 2


Device::Device(char *username, Connection *connection) {
    this->username = username;
    this->connections[connection->port] = connection;
}


bool UserStore::add_user(char *username) {
    if(this->users_devices.find(username) == this->users_devices.end()) {
        map<string, Device *> device_map;
        this->users_devices[username] = device_map;
        return true;
    }
    cout << "User " << username << " already registered." << endl;
    return false;
}

bool UserStore::register_device(char *username, Connection *connection) {
    map<string, Device *> device_map;
    try {
        device_map= this->users_devices.at(username);
    } catch (out_of_range) {
        cout << "User " << username << " is not registered" << endl;
        return false;
    }

    //  This is where we limit max devices online
    if (device_map.size() == USER_MAX_DEVICES) {
        cout << "User " << username << " reached maximum of simultaneously connected devices: " << USER_MAX_DEVICES << endl;
        cout << "Connection from device with address " << connection->address << " rejected." << endl;
        return false;
    }

    if(device_map.find(connection->address) == device_map.end()) {
        Device *device = new Device(username, connection);
        this->users_devices[username][connection->address] = device;
        connection->set_device(device);
        return true;
    }

    cout << "Device with address " << connection->address << " already registered" << endl;
    return true;
}


bool UserStore::register_connection(char *username, Connection *connection) {
    map<string, Device *> device_map = this->users_devices[username];

    if(device_map.find(connection->address) == device_map.end()) {
        return this->register_device(username, connection);
    } else {
        Device *device = device_map[connection->address];
        device->connections[connection->port] = connection;
        connection->set_device(device);
    }
    return true;
}


bool UserStore::unregister_connection(Connection *connection) {
    if (connection->device != NULL) {
        Device *device = connection->device;
        map<int, Connection *>::iterator iter = device->connections.find(connection->port);
        if (iter != device->connections.end()) {
            device->connections.erase(iter);
        }
    }
    return true;
}


vector<Device *> UserStore::get_user_devices(char *username) {
    vector<Session *> session_vec = this->users_sessions[username];
    vector<Device *> device_vec;
    for (int idx=0; idx < session_vec.size(); idx++) {
        device_vec.push_back(session_vec[idx]->device);
    }
    return device_vec;
}


Device * UserStore::get_device(char *username, string address) {
    try {
        return this->users_devices.at(username).at(address);
    } catch (out_of_range) {
        cout << "Cannot find device at address " << address << " from user " << username << endl;
    }
    return NULL;
}


vector<string> UserStore::get_connected_users() {
    vector<string> users;
    map<string, vector<Session *> > sessions = this->users_sessions;
    transform(
        sessions.begin(), sessions.end(), back_inserter(users),
        [](pair<string, vector<Session *> > p) { return p.first; }  // NOLINT
    );
    return users;
}


void UserStore::get_user_devices() {

}
