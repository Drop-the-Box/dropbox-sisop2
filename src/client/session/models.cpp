#include "models.hpp"


ClientContext::ClientContext(
    ConnectionManager *conn_manager, Inotify *file_monitor, string sync_dir,
    SessionType session_type, bool *is_listening_dir, bool *interrupt
) {
    this->conn_manager = conn_manager;
    this->session_type = session_type;
    this->sync_dir     = sync_dir;
    this->interrupt = interrupt;
    this->is_listening_dir = is_listening_dir;
    this->file_monitor = file_monitor;
};

ClientContext::~ClientContext() {
    free(this->conn_manager);
}

