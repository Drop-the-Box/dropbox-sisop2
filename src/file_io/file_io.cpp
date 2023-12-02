

#include "file_io.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <filesystem>

using namespace std;

FileHandler::FileHandler(const string& filename) : filename(filename) {}

FileHandler::~FileHandler() {
    close_file();
}

bool FileHandler::open_file() {
    file.open(filename);
    return file.is_open();
}

void FileHandler::close_file() {
    if (file.is_open()) {
        file.close();
    }
}

void FileHandler::read_file() {
    if (!file.is_open()) {
        cerr << "Error: File is not open.\n";
        return;
    }

    string line;
    while (getline(file, line)) {
        cout << line << endl;
    }
}


std::vector<std::string> FileHandler::list_files(const std::string& directory) {
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        files.push_back(entry.path().string());
    }
    return files;
}

std::string FileHandler::buff_file() {
    if (!file.is_open()) {
        cerr << "Error: File is not open.\n";
        return "";
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool FileHandler::delete_file() {
        close_file(); // Certifique-se de que o arquivo está fechado antes de excluí-lo


    if (remove(filename.c_str()) != 0) {
        cerr << "Error: Unable to delete the file.\n";
        return false;
    }

    cout << "File deleted successfully.\n";
    return true;
}

std::string FileHandler::get_digest_file() {
    // Adicione a lógica para calcular o digest (hash) do arquivo, por exemplo, usando OpenSSL ou Crypto++
    // Essa parte depende da biblioteca que você escolher para cálculos de hash.
    // Substitua o seguinte código com a implementação real do cálculo do hash.
    std::cerr << "Error: Hash calculation not implemented.\n";
    return "";
}

void FileHandler::listen_file() {
    // Adicione a lógica para ouvir alterações no arquivo, por exemplo, usando File System TS ou outras bibliotecas.
    // Essa parte depende da biblioteca que você escolher para operações de sistema de arquivos.
    std::cerr << "Error: File listening not implemented.\n";
}


















