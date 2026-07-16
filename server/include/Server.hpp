#pragma once

#include <enet/enet.h>
#include <iostream>

class Server{
    public:

        Server();
        ~Server();
        void run();

    private:

        ENetHost* server;
        ENetAddress address;

};