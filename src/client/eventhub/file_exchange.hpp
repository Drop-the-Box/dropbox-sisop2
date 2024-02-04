#ifndef CLIENT_FILEX_H
#define CLIENT_FILEX_H

#include "../../common/file_io/file_io.hpp"
#include "../session/session.hpp"
#include <sys/types.h>

class ClientFileSync {
    shared_ptr<ClientContext> context;
    shared_ptr<FileHandler>   file_handler;
    bool *interrupt;

public:
    ClientFileSync(shared_ptr<ClientContext> context, bool *interrupt);
    void loop();
    void get_event();
};

#endif
