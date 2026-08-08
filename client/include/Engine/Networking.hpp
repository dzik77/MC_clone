#pragma once
#include <enet/enet.h>
#include <string>
#include <iostream>
#include <PacketType.hpp>
#include <queue>
#include <glm/glm.hpp>
#include <deque>
#include <Engine/Player.hpp>
#include <IWorldAccesor.hpp>

struct SavedClientState {
    uint32_t sequenceID;
    glm::vec3 predictedPosition;
    float yaw,pitch;
    uint8_t input;
    float xoffset,yoffset;
    float dt;
};

class Networking {

    public:

        Networking();
        ~Networking();
        void initialize(std::queue<ChunkPacket> &receivedChunks, Player &player, IWorldAccessor &worldAccessor);
        void sendPlayerMovePacket(SavedClientState state);
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

        // client state
        Player &player;
        IWorldAccessor& worldAccessor;
        int currentSequenceID = 1;
        std::deque<SavedClientState> clientStateHistory;

};
