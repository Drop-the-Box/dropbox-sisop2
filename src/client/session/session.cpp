#include <cstdlib>
#include <iostream>
#include <plog/Log.h>
#include <pthread.h>
#include <signal.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "../../common/eventhub/models.hpp"
#include "../../common/file_io/file_io.hpp"
#include "../../common/session/models.hpp"
#include "../../common/socket_io/socket.hpp"
#include "../../common/vars.hpp"
#include "../eventhub/file_exchange.hpp"
#include "../eventhub/publisher.hpp"
#include "../eventhub/subscriber.hpp"
#include "session.hpp"

using namespace std;

bool interrupt = false;

void stop_execution(int signal) {
    PLOGD << "Caught signal: " << strsignal(signal) << "\n\n";
    interrupt = true;
}

ClientContext::ClientContext(
    string server_addr, int server_port, string username, string sync_dir, SessionType session_type) {
    this->server_addr  = server_addr;
    this->server_port  = server_port;
    this->username     = username;
    this->session_type = session_type;
    this->sync_dir     = sync_dir;
};

ClientSessionManager::ClientSessionManager(string address, int port, string username) {
    ::signal(SIGINT, stop_execution);
    string         sync_dir              = FileHandler::get_sync_dir(username);
    ClientContext *publisher_context     = new ClientContext(address, port, username, sync_dir, CommandPublisher);
    ClientContext *subscriber_context    = new ClientContext(address, port, username, sync_dir, CommandSubscriber);
    ClientContext *file_exchange_context = new ClientContext(address, port, username, sync_dir, FileExchange);

    pthread_t thread_pool[3];

    pthread_create(&thread_pool[0], NULL, this->handle_session, publisher_context);
    pthread_create(&thread_pool[1], NULL, this->handle_session, subscriber_context);
    pthread_create(&thread_pool[2], NULL, this->handle_session, file_exchange_context);

    pthread_exit(NULL);
};

void *ClientSessionManager::handle_session(void *context_ptr) {
    shared_ptr<ClientContext> context((ClientContext *)context_ptr);
    shared_ptr<Socket>        socket(new Socket(context->server_addr, context->server_port, &interrupt, Client, BUFFER_SIZE));

    unique_ptr<ClientSession> session(new ClientSession(context, socket));
    try {
        if (session->setup()) {
            PLOGI << "Session setup complete..." << endl;
            session->run();
            PLOGI << "Session teardown complete..." << endl;
        }
    } catch (const std::exception &exc) {
        PLOGE << "Terminated with error: " << exc.what() << endl;
    }
    PLOGI << "Closing socket " << socket->socket_fd << endl;
    socket->close(socket->socket_fd);
    pthread_exit(NULL);
}

ClientSession::ClientSession(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
    shared_ptr<SessionRequest> request(new SessionRequest(context->session_type, context->username));

    this->socket  = socket;
    this->request = request;
    this->context = context;
};

bool ClientSession::setup() {
    uint8_t           *bytes;
    uint8_t          **bytes_ptr = &bytes;
    size_t             sreq_size = request->to_bytes(bytes_ptr);
    unique_ptr<Packet> packet(new Packet(SessionInit, 1, sreq_size, sreq_size, bytes));
    int                bytes_sent = packet->send(socket, socket->socket_fd);
    if (bytes_sent < 0) {
        return false;
    }

    shared_ptr<uint8_t> msg((uint8_t *)calloc(BUFFER_SIZE, sizeof(uint8_t)));
    socket->get_message_sync(msg.get(), socket->socket_fd);
    unique_ptr<Packet> resp_packet(new Packet(msg.get()));
    if (resp_packet->type == EventMsg) {
        unique_ptr<Event> evt(new Event(resp_packet->payload));
        if (evt->type == SessionAccepted) {
            PLOGI << "Session accepted from server..." << endl;
            return true;
        }
        PLOGD << "Event detail: " << evt->message << endl;
    }
    free(bytes);
    return false;
}

void ClientSession::run() {
    PLOGI << "Running session..." << endl;
    PLOGI << "Session type: " << session_type_map.at(context->session_type) << endl;
    switch (context->session_type) {
    case CommandPublisher: {
        unique_ptr<ClientPublisher> publisher(new ClientPublisher(context, socket));
        publisher->loop();
        PLOGW << "Publisher loop finished..." << endl;
        break;
    }
    case CommandSubscriber: {
        unique_ptr<ClientSubscriber> subscriber(new ClientSubscriber(context, socket));
        subscriber->loop();
        PLOGW << "Subscriber loop finished..." << endl;
        break;
    }
    case FileExchange: {
        unique_ptr<ClientFileSync> file_sync(new ClientFileSync(context, socket));
        file_sync->loop();
        PLOGW << "File sync loop finished..." << endl;
        break;
    }
    default: {
        PLOGE << "Invalid session type " << session_type_map.at(context->session_type) << endl;
        break;
    }
    }
}
