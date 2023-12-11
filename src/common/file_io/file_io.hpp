#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "../socket_io/socket.hpp"

using namespace std;

enum SYNC_DIR_TYPE {
    DIR_SERVER,
    DIR_CLIENT
};

class FileMetadata {
public:
    string name;
    off_t  size;
    time_t modified_at;
    time_t accessed_at;
    time_t stat_changed_at;

    FileMetadata(uint8_t *bytes);
    FileMetadata(string name, string file_path);
    FileMetadata(string full_path);
    size_t to_bytes(uint8_t **bytes);
    bool   send(shared_ptr<Socket> socket, int channel);
};

class FileHandler {
    FILE                    *file_ptr;
    string                   file_path;
    shared_ptr<FileMetadata> metadata;

public:
    FileHandler(const string filename);
    ~FileHandler();
    void                  close();
    static vector<string> list_files(const string directory);
    bool                  send(shared_ptr<Socket> socket, int channel);
    bool                  delete_self();
    string                get_digest();
    void                  listen_file();
    long                  get_size();
    static string         get_sync_dir(string username, SYNC_DIR_TYPE mode = DIR_CLIENT);
    static bool           get_path_metadata(const string path, struct stat *metadata);
    static bool           path_exists(const string path);

private:
    string   filename;
    ifstream file;
};

#endif // FILE_HANDLER_HPP
