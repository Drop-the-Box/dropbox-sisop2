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


Device::Device(string username, shared_ptr<Connection> connection) {
    this->username = username;
    this->connections[connection->port] = connection;
}

UserStore::UserStore() {
    sem_init(&this->devices_lock, 0, 1); 
};

UserStore::~UserStore() {
    sem_destroy(&this->devices_lock);
}


bool UserStore::add_user(string username) {
    sem_wait(&this->devices_lock);
    bool user_added = false;
    if(this->users_devices.find(username) == this->users_devices.end()) {
        map<string, shared_ptr<Device> > device_map;
        this->users_devices[username] = device_map;
        sem_post(&this->devices_lock);
        user_added = true;
    } else {
        cout << "User " << username << " already registered." << endl;
    }
    sem_post(&this->devices_lock);
    return user_added;
}

bool UserStore::register_device(string username, shared_ptr<ServerContext> context) {
    sem_wait(&this->devices_lock);
    map<string, shared_ptr<Device> > device_map;

    shared_ptr<Connection> connection = context->connection;
    try {
        device_map = this->users_devices.at(username);
    } catch (out_of_range) {
        cout << "User " << username << " is not registered" << endl;
        sem_post(&this->devices_lock);
        return false;
    }

    //  This is where we limit max devices online
    if (device_map.size() == USER_MAX_DEVICES) {
        cout << "User " << username << " reached maximum of simultaneously connected devices: " << USER_MAX_DEVICES << endl;
        cout << "Connection from device with address " << connection->address << " rejected." << endl;
        sem_post(&this->devices_lock);
        return false;
    }

    if(device_map.find(connection->address) == device_map.end()) {
        shared_ptr<Device>device(new Device(username, connection));
        this->users_devices[username][connection->address] = device;
        context->set_device(device);
    } else {
        cout << "Device with address " << connection->address << " already registered" << endl;
    }

    sem_post(&this->devices_lock);
    return true;
}


bool UserStore::register_connection(string username, shared_ptr<ServerContext> context) {
    sem_wait(&this->devices_lock);
    map<string, shared_ptr<Device> > device_map = this->users_devices.at(username);

    shared_ptr<Device> device = context->device;
    shared_ptr<Connection> connection = context->connection;

    if(device_map.find(connection->address) == device_map.end()) {
        sem_post(&this->devices_lock);
        return this->register_device(username, context);
    } else {
        shared_ptr<Device>device = device_map[connection->address];
        device->connections[connection->port] = connection;
        context->device = device;
    }
    sem_post(&this->devices_lock);
    return true;
}


bool UserStore::unregister_connection(shared_ptr<ServerContext> context) {
    if (context->device.get() != NULL) {
        shared_ptr<Device> device = context->device;
        shared_ptr<Connection> connection = context->connection;
        sem_wait(&this->devices_lock);  // acquiring lock
        map<int, shared_ptr<Connection> >::iterator iter = context->device->connections.find(context->connection->port);
        if (iter != context->device->connections.end()) {
            context->device->connections.erase(iter);
        }
        sem_post(&this->devices_lock);
    }
    return true;
}


map<string, shared_ptr<Device> > UserStore::get_user_devices(string username) {
    try {
        return this->users_devices.at(username);
    } catch (out_of_range) {
        cerr << "Error! Cannot find username " << username << endl;
        throw;
    }
}


shared_ptr<Device> UserStore::get_device(string username, string address) {
    try {
        return this->users_devices.at(username).at(address);
    } catch (out_of_range) {
        cout << "Cannot find device at address " << address << " from user " << username << endl;
    }
    return NULL;
}


vector<string> UserStore::get_connected_users() {
    vector<string> users;
    map<string, vector<shared_ptr<Session> > > sessions = this->users_sessions;
    transform(
        sessions.begin(), sessions.end(), back_inserter(users),
        [](pair<string, vector<shared_ptr<Session>> > p) { return p.first; }  // NOLINT
    );
    return users;
}


void UserStore::get_user_devices() {

}


ServerContext::ServerContext(
    shared_ptr<Socket> socket, shared_ptr<Connection> connection, shared_ptr<UserStore> storage
) {
    this->socket = socket;
    this->connection = connection;
    this->storage = storage;
}


void ServerContext::set_device(shared_ptr<Device> device) {
    this->device = device;
}
