#pragma once

#include <enet/enet.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <string>
#include <World.hpp>

class Server{
    public:

        Server();
        ~Server();
        void run();

    private:

        std::atomic<bool> running;

        // networking
        ENetHost* server;
        ENetAddress address;

        // Console input handling

        std::thread consoleThread;
        std::queue<std::string> commandQueue;
        std::mutex commandMutex;

        void consoleInput();

        // World generation
        World world;

};