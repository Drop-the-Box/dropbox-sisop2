#include "subscriber.hpp"

ClientSubscriber::ClientSubscriber(shared_ptr<ClientContext> context, bool *interrupt) {
    this->context = context;
    this->interrupt = interrupt;
}

void ClientSubscriber::loop() {
    ConnectionManager *conn_manager = context->conn_manager;
    while (!*this->interrupt && !conn_manager->has_error(CommandPublisher)) {
        try {
            if(conn_manager->has_error(CommandPublisher)) continue;
        } 
        catch (ConnectionResetError &exc) {
        } catch (SocketTimeoutError &exc) {};
        //count++;
        // PLOG_INFO << "Subscriber loop count: " << count << endl;
        // PLOGI << "Subscriber channel: " << channel << endl;
        // sleep(5);
        // std::this_thread::sleep_for(std::chrono::seconds(5));
    }
};

void get_event(){

};
