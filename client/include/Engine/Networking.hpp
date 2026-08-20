#pragma once
#include <enet/enet.h>
#include <string>
#include <iostream>
#include <PacketType.hpp>
#include <queue>
#include <glm/glm.hpp>
#include <deque>
#include <IWorldAccesor.hpp>

class Player;
class World;
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
        void initialize(std::queue<ChunkPacket> &receivedChunks, Player &player, World &worldAccessor);
        void sendPlayerMovePacket(SavedClientState state);
        void sendPlaceBlockPacket(glm::ivec3 position, Block block);
        void sendDestroyBlockPacket(glm::ivec3 position);
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
        Player* player;
        World* worldAccessor;
        int currentSequenceID = 1;
        std::deque<SavedClientState> clientStateHistory;
        // If a rejection ever needs to replay more than this many buffered inputs (e.g. after a
        // network stall backs up clientStateHistory), replaying them all synchronously would stall
        // the main thread, so we snap to the server position and drop the backlog instead.
        static constexpr size_t MAX_CLIENT_STATE_HISTORY = 60;

};
