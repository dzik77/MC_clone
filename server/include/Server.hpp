#pragma once

#include <enet/enet.h>

inline bool operator==(const ENetAddress& lhs, const ENetAddress& rhs) {
    return lhs.host == rhs.host && lhs.port == rhs.port;
}
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
#include <ankerl/unordered_dense.h>
#include <Player.hpp>
#include <PhysicsEngine.hpp>
#include <chrono>

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

        // Server constants
        const int TARGET_TPS = 60; // Target ticks per second
        const std::chrono::milliseconds TICK_DURATION = std::chrono::milliseconds(1000 / TARGET_TPS);
        const int renderDistance = 32;

        std::atomic<bool> running;

        // networking
        ENetHost* server;
        ENetAddress address;
        void handlePacket(ENetEvent& event);

        // Client management
        struct ENetAdressHash {
            std::size_t operator()(const ENetAddress& k) const {
                return std::hash<uint32_t>()(k.host) ^ (std::hash<uint16_t>()(k.port) << 1);
            }
        };
        ankerl::unordered_dense::map<ENetAddress, Player, ENetAdressHash> connectedClients;

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