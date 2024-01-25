#ifndef CLIENT_SESSION_MODELS_H
#define CLIENT_SESSION_MODELS_H
#include "../../common/socket_io/conn_manager.hpp"


class ClientContext {
public:
    ConnectionManager *conn_manager;
    SessionType session_type;
    string      sync_dir;
    bool *interrupt;

    ClientContext(
        ConnectionManager *conn_manager, string sync_dir, SessionType session_type, bool *interrupt
    );
    ~ClientContext();

    inline string get_folder_path() { return this->sync_dir; }
    //  inline int    get_channel() { return this->channel; }
};
#endif
