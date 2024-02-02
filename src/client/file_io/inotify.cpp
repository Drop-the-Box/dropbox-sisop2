#include "../../common/file_io/file_io.hpp"
#include "inotify.hpp"

Inotify::Inotify(shared_ptr<ClientContext> context, const char *folder_path) {
    this->folder_path     = folder_path;
    this->file_descriptor = inotify_init();
    this->context = context;

    fcntl (this->file_descriptor, F_SETFL, fcntl (this->file_descriptor, F_GETFL) | O_NONBLOCK);
    if (this->file_descriptor == -1) {
        PLOGE << "Failed to initialize inotify" << endl;
        return;
    }

    this->watch_descriptor = inotify_add_watch(this->file_descriptor, this->folder_path, IN_MODIFY | IN_CREATE | IN_DELETE);
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

void Inotify::read_event() {
    PLOGD << "Inotify read event" << endl;

    char   buffer[BUFFER_LEN];
    string full_file_path;
    int    length = read(this->file_descriptor, buffer, BUFFER_LEN);
    if (length <= 0) return;

    unique_ptr<FileHandler> file_handler(new FileHandler(this->folder_path));
    if (length < 0) {
        PLOGE << "Failed to read" << endl;
    }
    PLOGD << "Read " << length << " bytes from inotify" << endl;
    struct inotify_event *event = (struct inotify_event *)&(buffer)[0];
    ConnectionManager* conn_manager = context->conn_manager;
    if (event->len && (event->mask & IN_CREATE || event->mask & IN_MODIFY)) {
        full_file_path = string(this->folder_path) + "/" + string(event->name);
        PLOGI << "The file " << full_file_path << " was created." << endl;
        file_handler->open(full_file_path);
        shared_ptr<Command> command(new Command(UploadFile, string(event->name)));
        while(true) {
            try {
                if(conn_manager->send_command(command, CommandPublisher)) {
                    if (file_handler->send(this->context->conn_manager, CommandPublisher)) {
                        PLOGI << "File " << full_file_path << " sent successfully." << endl;
                        break;
                    }
                }
            } catch (ConnectionResetError &exc){}
            catch(SocketTimeoutError &exc) {};

            PLOGE << "Error sending file " << full_file_path << "." << endl;
        }
        shared_ptr<Event> event = context->conn_manager->get_event(CommandPublisher);
        if (event == NULL) {
            PLOGE << "Cannot get reply from upload file command." << endl;
        }
        if (event->type == CommandSuccess) {
            PLOGI << "File " << full_file_path << "propagated to all backups." << endl; 
        } else {
            PLOGI << "Error uploading file. Repl: " << event->message << endl;
        }

        // } else if (event->mask & IN_MODIFY) {
        //     full_file_path = string(this->folder_path) + "/" + string(event->name);
        //     PLOGI << "The file " << full_file_path << " was modified." << endl;
        //     file_handler->open(full_file_path);
        //     if (file_handler->send(this->context->conn_manager, CommandPublisher)) {
        //         PLOGI << "File " << full_file_path << " sent successfully." << endl;
        //     } else {
        //         PLOGE << "Error sending file " << full_file_path << "." << endl;
        //     }
    } else if (event->mask & IN_DELETE) {
        full_file_path = string(this->folder_path) + "/" + string(event->name);
        shared_ptr<Command> command(new Command(DeleteFile, string(event->name)));
        if (context->conn_manager->send_command(command, CommandPublisher)) {
            PLOGI << "The file " << full_file_path << " was deleted." << endl;
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
