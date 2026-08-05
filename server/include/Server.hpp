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
#include <condition_variable>
#include <syncstream>

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
        std::vector<std::thread> chunkGenerationThreads;
        const int numChunkGenerationThreads = 5;
        const int chunkToGeneratePerThread = 20;
        World world;

        std::queue<ChunkToGenerate> chunksToGenerate;
        std::mutex chunksToGenerateMutex;
        std::condition_variable chunksToGenerateCV;

        std::queue<ChunkToSend> chunksToSend;
        std::mutex chunksToSendMutex;

        void generateChunks(int threadId);

};