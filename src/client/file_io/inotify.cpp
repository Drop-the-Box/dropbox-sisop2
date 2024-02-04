#include "../../common/file_io/file_io.hpp"
#include "inotify.hpp"


Inotify::Inotify(ConnectionManager *conn_manager, const string folder_path) {
    this->folder_path     = folder_path;
    this->file_descriptor = inotify_init();
    this->conn_manager    = conn_manager;

    this->file_handler = make_shared<FileHandler>(folder_path);

    vector<string> files = this->file_handler->list_files(folder_path);
    for (auto it = files.begin(); it != files.end(); it++) {
        this->add_file(*it);
    }

    fcntl (this->file_descriptor, F_SETFL, fcntl (this->file_descriptor, F_GETFL) | O_NONBLOCK);
    if (this->file_descriptor == -1) {
        PLOGE << "Failed to initialize inotify" << endl;
        return;
    }

    this->watch_descriptor = inotify_add_watch(this->file_descriptor, this->folder_path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
    if (this->watch_descriptor == -1) {
        PLOGE << "Failed to add watch" << endl;
        return;
    }
}

int Inotify::get_file_descriptor() {
    return this->file_descriptor;
}

int Inotify::get_watch_descriptor() {
    return this->watch_descriptor;
}

vector<string> Inotify::get_files() {
    vector<string> file_list;
    for(auto it = files.begin(); it != files.end(); it++) {
        file_list.push_back(it->first);
    }
    return file_list;
}

bool Inotify::add_file(string filename) {
    shared_ptr<FileMetadata> new_file(new FileMetadata(filename, this->folder_path + "/"));
    return this->add_file(new_file);
}

bool Inotify::add_file(shared_ptr<FileMetadata> new_file) {
    if (this->files.find(new_file->name) != this->files.end()) {
        shared_ptr<FileMetadata> current_file = this->files.at(new_file->name);
        if (new_file->size == current_file->size) {
            PLOGI << "File " << new_file->name << " is identical to current, no need to propagate" << endl;
            return false;
        }
    }
    this->files[new_file->name] = new_file;
    return true;
}

void Inotify::delete_file(string filename) {
    if (this->files.find(filename) != this->files.end()) {
        this->files.erase(filename);
        file_handler->delete_file(filename);
    }
}

void Inotify::read_event() {
    PLOGD << "Inotify read event" << endl;

    char   buffer[BUFFER_LEN];
    string full_file_path;
    int    length = read(this->file_descriptor, buffer, BUFFER_LEN);
    if (length <= 0) return;

    if (length < 0) {
        PLOGE << "Failed to read" << endl;
    }
    PLOGD << "Read " << length << " bytes from inotify" << endl;
    struct inotify_event *event = (struct inotify_event *)&(buffer)[0];
    if (event->len && (event->mask & IN_CREATE || event->mask & IN_MODIFY)) {
        sleep(3);
        if (!this->add_file(event->name)) {
            return;
        }
        full_file_path = string(this->folder_path) + "/" + string(event->name);
        PLOGI << "The file " << full_file_path << " was created." << endl;
        file_handler->open(full_file_path);
        shared_ptr<Command> command(new Command(UploadFile, string(event->name)));
        while(true) {
            try {
                if(conn_manager->has_error(CommandPublisher)) continue;
                bool cmd_result = conn_manager->send_command(command, CommandPublisher);
                if (!cmd_result) {
                    PLOGW << "Upload command failed. Retrying..." << endl;
                    continue;
                }
                bool file_result = file_handler->send(conn_manager, CommandPublisher);
                if (!file_result) {
                    PLOGW << "Failed sending file to server. Retrying... " << endl;
                    continue;
                }
                PLOGI << "File " << full_file_path << " sent successfully." << endl;
                break;
            } catch (ConnectionResetError &exc){
                PLOGW << "Connection to server resetted. Retrying operation..." << endl;
            }
            catch(SocketTimeoutError &exc) {
                PLOGW << "Operation to server timed out. Retrying operation..." << endl;
            };

            PLOGE << "Error sending file " << full_file_path << "." << endl;
        }
        shared_ptr<Event> event = conn_manager->get_event(CommandPublisher);
        if (event == NULL) {
            PLOGE << "Cannot get reply from upload file command." << endl;
        }
        if (event->type == CommandSuccess) {
            PLOGI << "File " << full_file_path << " propagated to all backups." << endl; 
        } else {
            PLOGI << "Error uploading file. Repl: " << event->message << endl;
        }
    } else if (event->mask & IN_DELETE) {
        full_file_path = string(this->folder_path) + "/" + string(event->name);
        if (this->files.find(event->name) == this->files.end()) {
            PLOGW << "File " << event->name << " has already been deleted" << endl;
        };
        shared_ptr<Command> command(new Command(DeleteFile, string(event->name)));
        while(true) {
            try {
                if(conn_manager->has_error(CommandPublisher)) continue;
                bool cmd_result = conn_manager->send_command(command, CommandPublisher);
                if (!cmd_result) {
                    PLOGW << "Delete command failed. Retrying..." << endl;
                    continue;
                }
                PLOGI << "The file " << full_file_path << " was deleted." << endl;
                this->delete_file(event->name);
                break;
            } catch (ConnectionResetError &exc){
                PLOGW << "Connection to server resetted. Retrying operation..." << endl;
            }
            catch(SocketTimeoutError &exc) {
                PLOGW << "Operation to server timed out. Retrying operation..." << endl;
            };
        }
        // file_handler->open(full_file_path);
        // if (fileHandler->send(this->socket, this->socket->socket_fd)) {
        // ver função de deletar arquivo no servidor
    }
    PLOGI << "Inotify read event end" << endl;
}

void Inotify::closeInotify() {
    inotify_rm_watch(this->file_descriptor, this->watch_descriptor);
    close(this->file_descriptor);
}
