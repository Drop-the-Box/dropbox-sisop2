#ifndef CLIENT_SESSION_MODELS_H
#define CLIENT_SESSION_MODELS_H
#include "../../common/socket_io/conn_manager.hpp"
#include "../file_io/inotify.hpp"


class ClientContext {
public:
    ConnectionManager *conn_manager;
    SessionType session_type;
    string      sync_dir;
    bool *is_listening_dir;
    bool *interrupt;
    Inotify *file_monitor;

    ClientContext(
        ConnectionManager *conn_manager, Inotify *file_monitor, string sync_dir,
        SessionType session_type, bool *is_listening_dir, bool *interrupt
    );
    ~ClientContext();

    inline string get_folder_path() { return this->sync_dir; }
    //  inline int    get_channel() { return this->channel; }
};
#endif
