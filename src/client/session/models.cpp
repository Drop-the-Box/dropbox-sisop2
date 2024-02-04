#include "models.hpp"


ClientContext::ClientContext(
    ConnectionManager *conn_manager, string sync_dir, SessionType session_type, bool *interrupt
) {
    this->conn_manager = conn_manager;
    this->session_type = session_type;
    this->sync_dir     = sync_dir;
    this->interrupt = interrupt;
};

ClientContext::~ClientContext() {
    free(this->conn_manager);
}

