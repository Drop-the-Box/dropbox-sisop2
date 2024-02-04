#include <cstdlib>
#include <iostream>
#include <plog/Log.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include "session.hpp"
#include "../../common/file_io/file_io.hpp"
#include "../eventhub/file_exchange.hpp"
#include "../eventhub/publisher.hpp"
#include "../eventhub/subscriber.hpp"

using namespace std;

bool interrupt = false;

void stop_execution(int signal) {
    PLOGW << "Caught signal: " << strsignal(signal) << "\n\n";
    interrupt = true;
}

ClientSessionManager::ClientSessionManager(string username) {
    ::signal(SIGINT, stop_execution);
    const string sync_dir = FileHandler::get_sync_dir(username);
    ConnectionManager *conn_manager = new ConnectionManager(username, &interrupt);
    Inotify *file_monitor = new Inotify(conn_manager, sync_dir);
    bool is_listening_dir = false;
    ClientContext *publisher_context     = new ClientContext(
        conn_manager, file_monitor, sync_dir, CommandPublisher, &is_listening_dir, &interrupt
    );
    ClientContext *subscriber_context    = new ClientContext(
        conn_manager, file_monitor, sync_dir, CommandSubscriber, &is_listening_dir, &interrupt
    );
    ClientContext *file_exchange_context = new ClientContext(
        conn_manager, file_monitor, sync_dir, FileExchange, &is_listening_dir, &interrupt
    );

    pthread_t thread_pool[3];

    pthread_create(&thread_pool[0], NULL, this->handle_session, publisher_context);
    pthread_create(&thread_pool[1], NULL, this->handle_session, subscriber_context);
    pthread_create(&thread_pool[2], NULL, this->handle_session, file_exchange_context);
    pthread_exit(NULL);
};




void *ClientSessionManager::handle_session(void *context_ptr) {
    shared_ptr<ClientContext> context((ClientContext *)context_ptr);

    unique_ptr<ClientSession> session(new ClientSession(context));
    try {
        session->run();
    } catch (UserInterruptError &exc) {
        context->conn_manager->stop_connections();
        PLOGI << "Ending session..." << endl;
    }
    pthread_exit(NULL);
}

ClientSession::ClientSession(shared_ptr<ClientContext> context) {
    this->context = context;
};


void ClientSession::run() {
    PLOGD << "Running session..." << endl;
    PLOGD << "Session type: " << session_type_map.at(context->session_type) << endl;
    switch (context->session_type) {
    case CommandPublisher: {
        unique_ptr<ClientPublisher> publisher(new ClientPublisher(context, &interrupt));
        PLOGI << "Publisher loop starting..." << endl;
        publisher->loop();
        PLOGI << "Publisher loop finished..." << endl;
        break;
    }
    case CommandSubscriber: {
        unique_ptr<ClientSubscriber> subscriber(new ClientSubscriber(context, &interrupt));
        PLOGI << "Subscriber loop starting..." << endl;
        subscriber->loop();
        PLOGI << "Subscriber loop finished..." << endl;
        break;
    }
    case FileExchange: {
        unique_ptr<ClientFileSync> file_sync(new ClientFileSync(context, &interrupt));
        PLOGI << "File sync loop starting..." << endl;
        file_sync->loop();
        PLOGI << "File sync loop finished..." << endl;
        break;
    }
    default: {
        PLOGE << "Invalid session type " << session_type_map.at(context->session_type) << endl;
        break;
    }
    }
}
