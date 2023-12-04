#ifndef USERLAND_CMN_MODELS_H
#define USERLAND_CMN_MODELS_H

#include <string>
#include <map>
#include<netinet/in.h>

using namespace std;

enum CommandType {
   UploadFile, DownloadFile, DeleteFile, ListServerFiles, ListLocalFiles, Exit
};

enum EventType {
    SessionAccepted, SessionRejected, FileCreated, FileDeleted 
};


static const map<string, CommandType> command_map = {
    { "upload", UploadFile },
    { "download", DownloadFile },
    { "delete", DownloadFile },
    { "list_server", ListServerFiles },
    { "list_client", ListLocalFiles }
};

class Command {
    public:
        CommandType type;
};

class Event {
    public:
        EventType type;
        string message;

        Event(uint8_t *bytes);
        Event(EventType type, string message);
        size_t to_bytes(uint8_t** bytes_ptr);
};
#endif
