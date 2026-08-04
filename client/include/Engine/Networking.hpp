#pragma once
#include <enet/enet.h>
#include <string>
#include <iostream>
#include <PacketType.hpp>
#include <queue>

class Networking {

    public:

        Networking();
        ~Networking();
        void initialize(std::queue<ChunkPacket> &receivedChunks);
        bool connectToServer(std::string ip);
        void pollEvents();
        void handleEvents(ENetEvent& event);

        ENetPeer* getServerPeer() const { return serverPeer; }

    private:

        // networking
        ENetHost* client;
        ENetAddress address;
        ENetPeer* serverPeer;

        //chunks packet
        std::queue<ChunkPacket>* receivedChunks;

};