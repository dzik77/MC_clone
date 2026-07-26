#pragma once

#include <enet/enet.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <string>
#include <World.hpp>
#include <PacketType.hpp>

struct ChunkToGenerate {
    int x;
    int z;
    ENetPeer* peer;
};
struct ChunkToSend {
    ChunkPacket chunkPacket;
    ENetPeer* peer;
};

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
        void handlePacket(ENetEvent& event);

        // Console input handling

        std::thread consoleThread;
        std::queue<std::string> commandQueue;
        std::mutex commandMutex;

        void consoleInput();

        // World generation
        std::thread chunkGenerationThread;
        World world;

        std::queue<ChunkToGenerate> chunksToGenerate;
        std::mutex chunksToGenerateMutex;

        std::queue<ChunkToSend> chunksToSend;
        std::mutex chunksToSendMutex;

        void generateChunks();

};