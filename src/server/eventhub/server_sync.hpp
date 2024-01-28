#include "../userland/models.hpp"

class BackupSubscriber {
    shared_ptr<ServerContext> context;
    shared_ptr<Socket> socket;
    int channel;
    bool *interrupt;
    
    public:
        BackupSubscriber(shared_ptr<ServerContext> context);
        void loop(); 
        shared_ptr<Command> receive_command();
};
