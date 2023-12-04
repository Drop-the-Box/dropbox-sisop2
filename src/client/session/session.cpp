#include <pthread.h>
#include <cstdlib>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <sys/stat.h>


#include "session.hpp"
#include "../../common/session/models.hpp"
#include "../../common/socket_io/socket.hpp"
#include "../../common/file_io/file_io.hpp"
#include "../../common/vars.hpp"
#include "../eventhub/publisher.hpp"
#include "../eventhub/subscriber.hpp"
#include "../eventhub/file_exchange.hpp"

using namespace std;


ClientContext::ClientContext(
    string server_addr, int server_port, string username, string sync_dir, SessionType session_type
) {
    this->server_addr = server_addr;
    this->server_port = server_port;
    this->username = username;
    this->session_type = session_type;
    this->sync_dir = sync_dir;
};


ClientSessionManager::ClientSessionManager(string address, int port, string username) {
    string sync_dir = FileHandler::get_sync_dir(username);
    ClientContext *publisher_context = new ClientContext(address, port, username, sync_dir, CommandPublisher);
    ClientContext *subscriber_context = new ClientContext(address, port, username, sync_dir, CommandSubscriber);
    ClientContext *file_exchange_context = new ClientContext(address, port, username, sync_dir, FileExchange);

    pthread_t thread_pool[3];

    pthread_create(&thread_pool[0], NULL, this->handle_session, publisher_context);
    pthread_create(&thread_pool[1], NULL, this->handle_session, subscriber_context);
    pthread_create(&thread_pool[2], NULL, this->handle_session, file_exchange_context);

    pthread_exit(NULL);
};


void *ClientSessionManager::handle_session(void *context_ptr) {
    shared_ptr<ClientContext> context((ClientContext *)context_ptr);
    shared_ptr<Socket> socket(new Socket(context->server_addr, context->server_port, Client, BUFFER_SIZE));

    unique_ptr<ClientSession> session(new ClientSession(context, socket));
    if (session->setup()) {
        session->run();
    }
    socket->close(socket->socket_fd);
    pthread_exit(NULL);
}


ClientSession::ClientSession(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    shared_ptr<SessionRequest> request(new SessionRequest(context->session_type, context->username));

    this->socket = socket;
    this->request = request;
    this->context = context;
};


bool ClientSession::setup() {
    uint8_t *bytes;
    uint8_t **bytes_ptr = &bytes;
    size_t sreq_size = request->to_bytes(bytes_ptr);
    unique_ptr<Packet> packet(new Packet(SessionInit, 1, sreq_size, sreq_size, bytes));
    int bytes_sent = packet->send(socket, socket->socket_fd);
    if (bytes_sent < 0) {
        return false;
    }

    uint8_t msg[socket->buffer_size];
    socket->get_message_sync(msg, socket->socket_fd);
    unique_ptr<Packet> resp_packet(new Packet(msg));
    if (resp_packet->type == EventMsg) {
        unique_ptr<Event> evt(new Event(resp_packet->payload));
        if (evt->type == SessionAccepted) {
            cout << "Session accepted from server..." << endl;
            return true;
        }
        cout << "Event detail: " << evt->message << endl;
    }
    free(bytes);
    return false;
}

void ClientSession::run() {
    switch (context->session_type) {
        case CommandPublisher: {
            unique_ptr<ClientPublisher> publisher(new ClientPublisher(context, socket));
            publisher->loop();
            break;
        }
        case CommandSubscriber: {
            unique_ptr<ClientSubscriber> subscriber(new ClientSubscriber(context, socket));
            subscriber->loop();
            break;
        }
        case FileExchange: {
            unique_ptr<ClientFileSync> file_sync(new ClientFileSync(context, socket));
            file_sync->loop();
            break;
        }
        default: {
            cerr << "Invalid session type " << session_type_map.at(context->session_type) << endl;
            break;
        }
    }
}
