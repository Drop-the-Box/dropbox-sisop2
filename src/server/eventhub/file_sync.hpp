#ifndef FILE_SYNC_H
#define FILE_SYNC_H

#include "../../common/file_io/file_io.hpp"
#include "../../common/socket_io/socket.hpp"
#include "../session/models.hpp"
#include "../userland/models.hpp"

#include <memory>

using namespace std;

class FileSync {
    shared_ptr<ServerContext> context;

public:
    FileSync(shared_ptr<ServerContext> context);
    void loop();
    void sync_all();
    void get_event();
};
#endif
