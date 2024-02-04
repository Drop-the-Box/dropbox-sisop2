#ifndef USERLAND_CMN_MODELS_H
#define USERLAND_CMN_MODELS_H

#include "../socket_io/socket.hpp"
#include <map>
#include <netinet/in.h>
#include <string>

using namespace std;

enum CommandType {
    SyncDir,
    Reconnect,
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
    FileDeleted,
    ServerAlive,
    ServerStatus,
    LeaderAccepted,
    LeaderRejected,
    CommandSuccess,
    CommandFailed,
    SyncDirFinished,
};

static const map<string, CommandType> command_map = {
    {"upload", UploadFile},
    {"download", DownloadFile},
    {"delete", DeleteFile},
    {"list_server", ListServerFiles},
    {"list_client", ListLocalFiles},
    {"exit", Exit}};

class Event {
public:
    EventType type;
    string    message;

    Event(uint8_t *bytes);
    Event(EventType type, string message);
    bool send(shared_ptr<Socket>, int channel);
};

class Command {
public:
    CommandType type;
    string arguments;

    Command(uint8_t *bytes);
    Command(CommandType type, string arguments);
    size_t to_bytes(uint8_t **bytes);
    bool send(shared_ptr<Socket>, int channel);
};

#endif
