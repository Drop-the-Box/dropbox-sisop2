#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

#include <string>
#include <fstream>

using namespace std;

class FileHandler {
public:
    FileHandler(const string& filename);
    ~FileHandler();



    bool open_file();
    void close_file();
    void read_file();
    std::vector<std::string> list_files(const std::string& directory);
    std::string buff_file();
    bool delete_file();
    std::string get_digest_file();
    void listen_file();

    

private:
    string filename;
    ifstream file;
};

#endif // FILE_HANDLER_HPP


