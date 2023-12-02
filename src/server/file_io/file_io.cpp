#ifndef FILE_IO_H
#define FILE_IO_H

#include "file_io.hpp"
#include <iostream>
#include <sys/stat.h>
#include <string.h>
#include<netinet/in.h>

using namespace std;


FileMetadata::FileMetadata(uint8_t *bytes) {
    uint8_t *cursor = bytes;
    size_t uint16_s = sizeof(uint16_t);
    uint16_t prop;
    copy(cursor, cursor+uint16_s, &prop);
    this->size = (size_t)ntohs(prop);
    cursor += uint16_s;
    copy(cursor, cursor+sizeof(time_t), &prop);
    this->accessed_at = (time_t)ntohs(prop);
    cursor += sizeof(time_t);
    copy(cursor, cursor+sizeof(time_t), &prop);
    this->modified_at = (time_t)ntohs(prop);
    cursor += sizeof(time_t);
    copy(cursor, cursor+sizeof(time_t), &prop);
    this->stat_changed_at = (time_t)ntohs(prop);
    cursor += sizeof(time_t);
    copy(cursor, cursor+255, this->name);
}


FileMetadata::FileMetadata(string filename, string file_path) {
    struct stat stat_data;
    string full_path = file_path + filename;
    if (stat(full_path.c_str(), &stat_data) == 0) {
        const char *name = filename.c_str();
        std::copy(name, name+strlen(name), this->name);
        this->size = stat_data.st_size;
        this->accessed_at = stat_data.st_atim.tv_sec;
        this->modified_at = stat_data.st_mtim.tv_sec;
        this->stat_changed_at = stat_data.st_ctim.tv_sec;
    } else {
        cout << "Error loading file " << filename << " metadata: " << strerror(errno) << endl;
        throw;
    }
}


size_t FileMetadata::to_bytes(uint8_t **bytes_ptr) {
    size_t packet_size = strlen(this->name) * sizeof(char);
    packet_size += sizeof(this->size);
    packet_size += sizeof(this->accessed_at);
    packet_size += sizeof(this->modified_at);
    packet_size += sizeof(this->stat_changed_at);
    *bytes_ptr = (uint8_t *)malloc(packet_size);

    uint8_t *cursor = *bytes_ptr;
    uint16_t prop = htons(this->size);
    size_t time_s = sizeof(time_t);
    copy(cursor, cursor+sizeof(this->size), &prop);
    cursor += sizeof(this->size);
    prop = htons(this->accessed_at);
    copy(cursor, cursor+time_s, &prop);
    cursor += time_s;
    prop = htons(this->modified_at);
    copy(cursor, cursor+time_s, &prop);
    cursor += time_s;
    prop = htons(this->stat_changed_at);
    copy(cursor, cursor+time_s, &prop);
    cursor += time_s;
    copy(cursor, cursor+strlen(this->name), this->name);
    return packet_size;
}


#endif
