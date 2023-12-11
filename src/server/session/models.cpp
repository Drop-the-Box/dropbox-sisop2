#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include "../../common/session/models.hpp"
#include "models.hpp"

Connection::Connection(char *address, int port, int channel) {
    this->address = string(address);
    this->port    = port;
    this->channel = channel;
}

void Connection::set_thread_id(pthread_t *thread_id) {
    this->thread_id = thread_id;
}

void Connection::set_session_type(SessionType session_type) {
    this->session_type = session_type;
}

string Connection::get_full_address() {
    ostringstream oss;
    oss << this->address << ":" << this->port;
    std::string conn_info = oss.str();
    return conn_info;
}
