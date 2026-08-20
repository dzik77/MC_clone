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

class Server{
    public:

        Server();
        ~Server();
        void run();

    private:

        // Server constants
        const int TARGET_TPS = 60; // Target ticks per second
        const std::chrono::milliseconds TICK_DURATION = std::chrono::milliseconds(1000 / TARGET_TPS);

        std::atomic<bool> running;

        // networking
        ENetHost* server;
        ENetAddress address;
        void handlePacket(ENetEvent& event);
        void sendUnloadChunkColumnPacket(ENetPeer* peer, glm::ivec2 chunkColumnPos);

        // Client management
        struct ENetAdressHash {
            std::size_t operator()(const ENetAddress& k) const {
                return std::hash<uint32_t>()(k.host) ^ (std::hash<uint16_t>()(k.port) << 1);
            }
        };
        ankerl::unordered_dense::map<ENetAddress, Player, ENetAdressHash> connectedClients;
        ankerl::unordered_dense::set<ENetAddress, ENetAdressHash> newlyConnectedClients; // Set of newly connected clients to send initial data

        // Console input handling

        std::thread consoleThread;
        std::queue<std::string> commandQueue;
        std::mutex commandMutex;

        void consoleInput();

        // World generation
        static constexpr int maxChunkColumnsSentPerTick = 50; // Maximum number of chunks to send per tick
        int renderDistance = 32;
        std::vector<glm::ivec2> chunksToLoadOffset;
        void setRenderDistance(int distance);
        World world;

};