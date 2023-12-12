#ifndef USERLAND_CMN_MODELS_H
#define USERLAND_CMN_MODELS_H

#include "../socket_io/socket.hpp"
#include <map>
#include <netinet/in.h>
#include <string>

using namespace std;

enum CommandType {
    UploadFile,
    DownloadFile,
    DeleteFile,
    ListServerFiles,
    ListLocalFiles,
    Exit
};

enum EventType {
    SessionAccepted,
    SessionRejected,
    FileCreated,
    FileDeleted
};

static const map<string, CommandType> command_map = {
    {"upload", UploadFile},
    {"download", DownloadFile},
    {"delete", DeleteFile},
    {"list_server", ListServerFiles},
    {"list_client", ListLocalFiles},
    {"exit", Exit}};

class Command {
public:
    CommandType type;
};

class Event {
public:
    EventType type;
    string    message;

    Event(uint8_t *bytes);
    Event(EventType type, string message);
    bool send(shared_ptr<Socket>, int channel);
};
#endif
