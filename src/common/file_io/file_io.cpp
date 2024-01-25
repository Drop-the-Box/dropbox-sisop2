#include <dirent.h>
#include <iostream>
#include <plog/Log.h>
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#include "file_io.hpp"

#include "../vars.hpp"

using namespace std;

FileMetadata::FileMetadata(uint8_t *bytes) {
    uint8_t *cursor = bytes;

    off_t size_prop;
    memmove(&size_prop, cursor, sizeof(off_t));
    this->size = (off_t)ntohl(size_prop);
    cursor += sizeof(off_t);

    // time_t t_prop;
    // size_t time_s = sizeof(time_t);
    // memmove(&t_prop, cursor, time_s);
    // this->accessed_at = (time_t)ntohs(t_prop);
    // cursor += time_s;

    // memmove(&t_prop, cursor, time_s);
    // this->modified_at = (time_t)ntohs(t_prop);
    // cursor += time_s;

    // memmove(&t_prop, cursor, time_s);
    // this->stat_changed_at = (time_t)ntohs(t_prop);
    // cursor += time_s;

    uint16_t prop;
    size_t   uint16_s = sizeof(uint16_t);
    memmove(&prop, cursor, uint16_s);
    uint16_t name_size = ntohs(prop);
    cursor += uint16_s;

    int max_name_size = Packet::get_max_payload_size() - sizeof(off_t) - uint16_s;
    if (name_size > max_name_size) {
        PLOGE << "Buffer overflow: " << name_size - max_name_size << " bigger than the buffer" << endl;
        name_size = max_name_size;
    }

    uint8_t *name = (uint8_t *)calloc(name_size, sizeof(char));
    memmove(name, cursor, name_size);
    this->name = string((char *)name, (size_t)name_size);
}

FileMetadata::FileMetadata(string filename, string file_path) {
    struct stat stat_data;
    string      full_path = file_path + filename;
    if (stat(full_path.c_str(), &stat_data) == 0) {
        this->name            = filename;
        this->size            = stat_data.st_size;
        this->accessed_at     = stat_data.st_atim.tv_sec;
        this->modified_at     = stat_data.st_mtim.tv_sec;
        this->stat_changed_at = stat_data.st_ctim.tv_sec;
    } else {
        PLOGE << "Error loading file " << filename << " metadata: " << strerror(errno) << endl;
        throw;
    }
}

FileMetadata::FileMetadata(string full_path) {
}

size_t FileMetadata::to_bytes(uint8_t **bytes_ptr) {
    size_t   time_s    = sizeof(time_t);
    size_t   uint16_s  = sizeof(uint16_t);
    uint16_t name_size = this->name.size() + 1;

    size_t packet_size = sizeof(off_t);
    // packet_size += time_s;
    // packet_size += time_s;
    // packet_size += time_s;
    packet_size += uint16_s;
    packet_size += name_size;

    *bytes_ptr      = (uint8_t *)calloc(packet_size, sizeof(uint8_t));
    uint8_t *cursor = *bytes_ptr;

    off_t enc_size = htonl(this->size);
    memmove(cursor, &enc_size, sizeof(off_t));
    cursor += sizeof(off_t);

    // time_t prop_t = htonl(this->accessed_at);
    // memmove(cursor, &prop_t, time_s);
    // cursor += time_s;

    // prop_t = htonl(this->modified_at);
    // memmove(cursor, &prop_t, time_s);
    // cursor += time_s;

    // prop_t = htonl(this->stat_changed_at);
    // memmove(cursor, &prop_t, time_s);
    // cursor += time_s;
    //

    uint16_t name_size_enc = htons(name_size);
    memmove(cursor, &name_size_enc, uint16_s);
    cursor += uint16_s;

    memmove(cursor, this->name.c_str(), name_size);
    return packet_size;
}



FileHandler::FileHandler(const string work_dir){
    this->work_dir = work_dir;
    this->file_ptr = NULL;
    this->metadata = NULL;
}

FileHandler::~FileHandler() {
    if (this->file_ptr != NULL)
        this->close();
}

void FileHandler::open(const string file_path) {
    this->file_path = file_path;
    this->file_ptr  = fopen(file_path.c_str(), "rb");
    regex  rgx("^(.*\\/)([^\\/]+)$");
    smatch matches;
    regex_search(file_path, matches, rgx);
    string folder_path = matches[1];
    // PLOGD << "path: " << folder_path << endl;
    string filename = matches[2];
    // PLOGD << "name: " << filename << endl;
    shared_ptr<FileMetadata> file_metadata(new FileMetadata(filename, folder_path));
    this->metadata = file_metadata;
}

void FileHandler::close() {
    fclose(this->file_ptr);
}

std::vector<string> FileHandler::list_files(const string directory) {
    struct dirent *entry;
    vector<string> files;

    DIR *dir_ptr = opendir(directory.c_str());
    if (dir_ptr != NULL) {
        while ((entry = readdir(dir_ptr))) {
            if (entry->d_type == DT_REG) {
                files.push_back(string(entry->d_name));
            }
        }
        closedir(dir_ptr);
    } else {
        perror("Couldn't open the directory");
        PLOGE << "Cannot open directory " << directory << " Reason: " << strerror(errno) << endl;
    }
    return files;
}

bool FileHandler::send(ConnectionManager *conn_manager, SessionType session_type) {
    off_t file_size = this->metadata->size;
    PLOGD << "File size: " << file_size << endl;
    int   seq_index        = 1;
    int   file_buf_size    = Packet::get_max_payload_size();
    int   file_bytes_read  = 0;
    int   total_bytes_sent = 0;
    float percentage       = 0;

    uint8_t *file_buf = (uint8_t *)calloc(file_buf_size, sizeof(uint8_t));
    while(true) {
        try {
            this->metadata->send(conn_manager, session_type);
        } catch (ConnectionResetError &exc) {
            continue;
        }
        PLOGW << "teste: " << this->file_ptr << endl;
        if (this->file_ptr != NULL) {
            PLOGI << "Sending file " << this->metadata->name << endl;
            while ((file_bytes_read = fread(file_buf, sizeof(uint8_t), file_buf_size, this->file_ptr)) > 0) {
                PLOGD << "Chunk index: " << seq_index << endl;

                shared_ptr<Packet> packet(new Packet(FileChunk, seq_index, file_size, file_bytes_read, file_buf));

                PLOGD << "Total file size: " << packet->total_size << endl;
                try {
                    conn_manager->send(packet, session_type);
                } catch (ConnectionResetError &exc) {
                    continue;
                }
                total_bytes_sent += file_bytes_read;
                percentage = (float)(total_bytes_sent / (float)file_size);

                PLOGD << "Transfering  " << this->metadata->name << ": " << percentage * 100 << " %" << endl;

                memset(file_buf, 0, file_bytes_read);
                seq_index += 1;
            }
        }
      break;
    } 
    free(file_buf);
    return true;
}

bool FileHandler::send(shared_ptr<Socket> socket, int channel) {
    this->metadata->send(socket, channel);
    off_t file_size = this->metadata->size;
    PLOGD << "File size: " << file_size << endl;
    int   seq_index        = 1;
    int   file_buf_size    = Packet::get_max_payload_size();
    int   file_bytes_read  = 0;
    int   total_bytes_sent = 0;
    float percentage       = 0;

    uint8_t *file_buf = (uint8_t *)calloc(file_buf_size, sizeof(uint8_t));
    PLOGW << "teste: " << this->file_ptr << endl;
    if (this->file_ptr != NULL) {
        PLOGI << "Sending file " << this->metadata->name << endl;
        while ((file_bytes_read = fread(file_buf, sizeof(uint8_t), file_buf_size, this->file_ptr)) > 0) {
            PLOGD << "Chunk index: " << seq_index << endl;

            unique_ptr<Packet> packet(new Packet(FileChunk, seq_index, file_size, file_bytes_read, file_buf));

            PLOGD << "Total file size: " << packet->total_size << endl;

            packet->send(socket, channel);
            total_bytes_sent += file_bytes_read;
            percentage = (float)(total_bytes_sent / (float)file_size);

            PLOGD << "Transfering  " << this->metadata->name << ": " << percentage * 100 << " %" << endl;

            memset(file_buf, 0, file_bytes_read);
            seq_index += 1;
        }
    }
    free(file_buf);
    return true;
}

bool FileMetadata::send(ConnectionManager *conn_manager, SessionType session_type) {
    uint8_t           *bytes;
    int                bsize = this->to_bytes(&bytes);
    shared_ptr<Packet> packet(new Packet(FileMetadataMsg, 1, bsize, bsize, bytes));
    conn_manager->send(packet, session_type);
    return true;
}

bool FileMetadata::send(shared_ptr<Socket> socket, int channel) {
    uint8_t           *bytes;
    int                bsize = this->to_bytes(&bytes);
    unique_ptr<Packet> packet(new Packet(FileMetadataMsg, 1, bsize, bsize, bytes));
    packet->send(socket, channel);
    return true;
}

bool FileHandler::delete_self() {
    this->close();

    if (remove(filename.c_str()) != 0) {
        PLOGE << "Error: Unable to delete the file.\n";
        return false;
    }

    PLOGI << "File " << this->filename << " deleted successfully.\n";
    return true;
}

long FileHandler::get_size() {
    struct stat file_status;
    if (stat(this->file_path.c_str(), &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}

bool FileHandler::get_path_metadata(const string path, struct stat *metadata) {
    return stat(path.c_str(), metadata) == 0;
}

bool FileHandler::path_exists(const string path) {
    struct stat metadata;
    return FileHandler::get_path_metadata(path, &metadata);
}

string FileHandler::get_digest() {
    PLOGE << "Error: Hash calculation not implemented.\n";
    return "";
}

void FileHandler::listen_file() {
    PLOGE << "Error: File listening not implemented.\n";
}

string FileHandler::get_sync_dir(string username, SYNC_DIR_TYPE mode) {
    ostringstream sync_dir;
    if (mode == DIR_SERVER) {
        sync_dir << "./srv_sync_dir/";
    } else {
        sync_dir << "./sync_dir/";
    }
    if (!FileHandler::path_exists(sync_dir.str())) {
        int main_dir_result = mkdir(sync_dir.str().c_str(), 0755);
        PLOGI << "Creating main sync dir " << sync_dir.str() << endl;
    }
    sync_dir << username;
    if (!FileHandler::path_exists(sync_dir.str())) {
        PLOGI << "Creating sync dir " << sync_dir.str() << endl;
        int dir_result = mkdir(sync_dir.str().c_str(), 0755);
        if (dir_result != 0 && errno != EEXIST) {
            PLOGE << "Cannot create directory: " << sync_dir.str() << endl;
            throw;
        }
    }
    return sync_dir.str();
};

void FileHandler::receive_file(
    string work_dir, shared_ptr<FileMetadata> metadata,
    ConnectionManager *conn_manager, SessionType kind
) {
    char buffer[BUFFER_SIZE];
    int  total_bytes = 0;
    int  collected_bytes;
    int  iteration;
    PLOGI << "Waiting for file metadata" << endl;
    int chars_read = 0;
    while(true) {
        try {
            chars_read = conn_manager->get_message((uint8_t *)buffer, kind);
            PLOGI << "Received file metadata" << endl;
            while (chars_read != 0) {
                PLOGD << "Chars read: " << chars_read << endl;
                collected_bytes = 0;
                iteration       = 1;
                chars_read      = 0;
                if (chars_read < 0)
                    printf("ERROR reading from socket\n");
                unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
                if (packet->type != FileMetadataMsg) {
                    PLOGE << "Invalid packet type. Expected file metadata, received: "
                        << packet->type << endl;
                    break;
                }
                //unique_ptr<FileMetadata> metadata(new FileMetadata(packet->payload));
                metadata = make_shared<FileMetadata>(packet->payload);
                PLOGI << "Received file " << metadata->name << endl;
                total_bytes = metadata->size;
                printf("File size: %ld\n", (long)total_bytes);
                ostringstream oss;
                oss << work_dir << "/" << metadata->name;
                string filepath = oss.str();
                PLOGI << "Storing file in " << filepath.c_str() << endl;
                FILE *file_output = fopen(filepath.c_str(), "wb");

                while (collected_bytes < total_bytes) {
                    chars_read = conn_manager->get_message((uint8_t *)buffer, kind);
                    PLOGD << "Chars read: " << chars_read << endl;
                    if (chars_read < -1) {
                        printf("ERROR reading from socket\n");
                        break;
                    }
                    PLOGD << "Iteration: " << iteration << endl;
                    iteration += 1;
                    unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
                    if (packet->type != FileChunk) {
                        PLOGE << "Invalid file chunk: " << endl;
                        PLOGD << "Packet type: " << packet->type << endl;
                        PLOGD << "Packet seq idx: " << packet->seq_index << endl;
                        PLOGD << "Packet total size: " << packet->total_size << endl;
                        PLOGD << "Packet payload size: " << packet->payload_size << endl;
                        PLOGD << "Packet payload: " << packet->payload << endl;
                        return;
                    }
                    fwrite(packet->payload, 1, packet->payload_size * sizeof(uint8_t), file_output);
                    PLOGD << "Received packet type " << packet->type << endl;
                    PLOGD << "File size: " << (long)packet->total_size << endl;
                    PLOGD << "Chunk size: " << packet->payload_size << endl;
                    PLOGD << "Chunk index: " << packet->seq_index << endl;
                    collected_bytes += packet->payload_size;
                    PLOGD << "Collected bytes: " << collected_bytes << endl;
                    total_bytes = packet->total_size;
                    memset(buffer, 0, BUFFER_SIZE);
                }
                if (file_output != NULL) {
                    fclose(file_output);
                }
                chars_read = conn_manager->get_message((uint8_t *)buffer, kind);
            }
        } catch (ConnectionResetError &exc) {
            PLOGD << "Failed receiving metadata due to connection reset. Retrying..." << endl;
        }
        break;
    }
}

void FileHandler::receive_file(string work_dir, shared_ptr<FileMetadata> metadata, shared_ptr<Socket> socket, int channel) {
    char buffer[BUFFER_SIZE];
    int  total_bytes = 0;
    int  collected_bytes;
    int  iteration;
    PLOGI << "Waiting for file metadata" << endl;
    int chars_read = socket->get_message_sync((uint8_t *)buffer, channel);
    PLOGI << "Received file metadata" << endl;
    while (chars_read != 0) {
        PLOGD << "Chars read: " << chars_read << endl;
        collected_bytes = 0;
        iteration       = 1;
        chars_read      = 0;
        if (chars_read < 0)
            printf("ERROR reading from socket\n");
        Packet *packet = new Packet((uint8_t *)buffer);
        if (packet->type != FileMetadataMsg) {
            PLOGE << "Invalid packet type. Expected file metadata, received: " << packet->type << endl;
            break;
        }
        //unique_ptr<FileMetadata> metadata(new FileMetadata(packet->payload));
        metadata = make_shared<FileMetadata>(packet->payload);
        PLOGI << "Received file " << metadata->name << endl;
        total_bytes = metadata->size;
        printf("File size: %ld\n", (long)total_bytes);
        ostringstream oss;
        oss << work_dir << "/" << metadata->name;
        string filepath = oss.str();
        PLOGI << "Storing file in " << filepath.c_str() << endl;
        FILE *file_output = fopen(filepath.c_str(), "wb");

        while (collected_bytes < total_bytes) {
            chars_read = socket->get_message_sync((uint8_t *)buffer, channel);
            PLOGD << "Chars read: " << chars_read << endl;
            if (chars_read < 0) {
                printf("ERROR reading from socket\n");
                break;
            }
            PLOGD << "Iteration: " << iteration << endl;
            iteration += 1;
            unique_ptr<Packet> packet(new Packet((uint8_t *)buffer));
            if (packet->type != FileChunk) {
                PLOGE << "Invalid file chunk: " << endl;
                PLOGD << "Packet type: " << packet->type << endl;
                PLOGD << "Packet seq idx: " << packet->seq_index << endl;
                PLOGD << "Packet total size: " << packet->total_size << endl;
                PLOGD << "Packet payload size: " << packet->payload_size << endl;
                PLOGD << "Packet payload: " << packet->payload << endl;
                return;
            }
            fwrite(packet->payload, 1, packet->payload_size * sizeof(uint8_t), file_output);
            PLOGD << "Received packet type " << packet->type << endl;
            PLOGD << "File size: " << (long)packet->total_size << endl;
            PLOGD << "Chunk size: " << packet->payload_size << endl;
            PLOGD << "Chunk index: " << packet->seq_index << endl;
            collected_bytes += packet->payload_size;
            PLOGD << "Collected bytes: " << collected_bytes << endl;
            total_bytes = packet->total_size;
            bzero(buffer, BUFFER_SIZE);
        }
        if (file_output != NULL) {
            fclose(file_output);
        }
        chars_read = socket->get_message_sync((uint8_t *)buffer, channel);
    }
}
