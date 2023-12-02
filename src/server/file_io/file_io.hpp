#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

using namespace std;

class FileMetadata {
    public:
        char name[255];
        size_t size;
        time_t modified_at;
        time_t accessed_at;
        time_t stat_changed_at;

        FileMetadata(uint8_t *bytes);
        FileMetadata(string name, string file_path);
        size_t to_bytes(uint8_t **bytes);
};
