#include "publisher.hpp"

using namespace std;

// ClientPublisher::ClientPublisher(shared_ptr<ClientContext> context, shared_ptr<Socket> socket) {
//     this->context = context;
//     this->socket  = socket;
// }

// void ClientPublisher::loop() {

//     shared_ptr<Inotify> inotify = make_shared<Inotify>(this->context, this->socket);
//     while (!*socket->interrupt) {
//         inotify->read_event();
//     }
// };

// void ClientPublisher::send_event(){

// };

#include "publisher.hpp"

ClientPublisher::ClientPublisher(ClientContext *context, Socket *socket) {
    this->context = context;
    this->socket  = socket;
}

void ClientPublisher::loop() {
    Inotify inotify(context, socket);

    while (!*(socket->interrupt)) {
        inotify.read_event();
    }
}

void ClientPublisher::send_event(const std::string &event_message) {
    // Implemente aqui a lógica para enviar o evento ao servidor.
    // Utilize this->socket para acessar o socket e enviar dados.
    // Considere usar algum formato para representar o evento, como JSON.
    // Exemplo: this->socket->send(event_message);
}

// auto inotify = std::make_shared<Inotify>(context.get(), socket.get());
