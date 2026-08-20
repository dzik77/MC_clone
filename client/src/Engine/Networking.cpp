#include <Engine/Networking.hpp>
#include <Engine/Player.hpp>
#include <WorldGeneration/World.hpp>
#include <ChunkCompression.hpp>

Networking::Networking() : client(nullptr), serverPeer(nullptr), receivedChunks(nullptr), player(nullptr), worldAccessor(nullptr) {}

Networking::~Networking() {
    if (client) {
        enet_host_destroy(client);
    }
}

void Networking::initialize(std::queue<ChunkPacket> &receivedChunks, Player &player, World &worldAccessor) {
    this->receivedChunks = &receivedChunks;
    this->player = &player;
    this->worldAccessor = &worldAccessor;
    // network initialization
    client = enet_host_create(nullptr, 1, 2, 0, 0);
    if (client == nullptr) {
        throw std::runtime_error("Failed to create ENet client host.");
    }
}

void Networking::sendPlayerMovePacket(SavedClientState state) {

    state.sequenceID = currentSequenceID++;

    PlayerMovePacket packet;
    packet.sequenceID = state.sequenceID;
    packet.position = state.predictedPosition;

    ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(PlayerMovePacket), 0);
    enet_peer_send(serverPeer, 1, enetPacket);

    // Mask out mouse buttons (left: 0b10000000, right: 0b1000000) so we don't replay block breaking/placing
    state.input &= ~(0b11000000);
    clientStateHistory.push_back(state);

}


void Networking::sendPlaceBlockPacket(glm::ivec3 position, Block block) {

    PlaceBlockPacket packet;
    packet.position = position;
    packet.block = block;

    ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(PlaceBlockPacket), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(serverPeer, 0, enetPacket);

}

void Networking::sendDestroyBlockPacket(glm::ivec3 position) {

    DestroyBlockPacket packet;
    packet.position = position;

    ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(DestroyBlockPacket), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(serverPeer, 0, enetPacket);

}

bool Networking::connectToServer(std::string ip)
{

    enet_address_set_host(&address, ip.c_str());
    address.port = 1234;

    serverPeer = enet_host_connect(client, &address, 2, 0);
    if (serverPeer == nullptr) {
        std::cerr << "No available peers for initiating an ENet connection." << std::endl;
        return false;
    }

    std::cout << "Attempting to connect to server at " << ip << ":" << address.port << "..." << std::endl;

    ENetEvent event;
    // Wait up to 5 seconds for the connection attempt to succeed
    if (enet_host_service(client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        std::cout << "Successfully connected to server at " << ip << ":" << address.port << std::endl;
        enet_host_flush(client); // Ensure all packets are sent (3 step handshake)
        return true;
    }
    else
    {
        // Connection failed or timed out
        enet_peer_reset(serverPeer);
        std::cout << "Failed to connect to server at " << ip << ":" << address.port << std::endl;
        return false;
    }
}

void Networking::pollEvents()
{

    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "Connected to server." << std::endl;
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                handleEvents(event);
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnected from server." << std::endl;
                break;
            default:
                break;
        }
    }

}

void Networking::handleEvents(ENetEvent& event) {
    
    PacketType packetType = static_cast<PacketType>(event.packet->data[0]);
    switch(packetType) {
        case PacketType::Chunk: {
            if (event.packet->dataLength < sizeof(ChunkPacketHeader) + 1) {
                std::cerr << "Warning: Invalid ChunkPacket length: " << event.packet->dataLength << std::endl;
                break;
            }
            ChunkPacketHeader* header = reinterpret_cast<ChunkPacketHeader*>(event.packet->data);
            ChunkPacket chunkPacket;
            chunkPacket.x = header->x;
            chunkPacket.y = header->y;
            chunkPacket.z = header->z;
            ChunkCompression::decodeChunk(event.packet->data + sizeof(ChunkPacketHeader),
                                           event.packet->dataLength - sizeof(ChunkPacketHeader),
                                           chunkPacket.chunkBlocks);
            receivedChunks->push(chunkPacket);
            break;
        }
        case PacketType::ServerAuthoritativeState: {
            if (event.packet->dataLength != sizeof(ServerAuthoritativeStatePacket)) {
                std::cerr << "Warning: Invalid ServerAuthoritativeStatePacket length: " << event.packet->dataLength << std::endl;
                break;
            }
            ServerAuthoritativeStatePacket* serverState = reinterpret_cast<ServerAuthoritativeStatePacket*>(event.packet->data);

            float lastYaw = player->getYaw();
            float lastPitch = player->getPitch();
            while(!clientStateHistory.empty() && clientStateHistory.front().sequenceID <= serverState->lastProcessedInputSequenceID) {
                lastYaw = clientStateHistory.front().yaw;
                lastPitch = clientStateHistory.front().pitch;
                clientStateHistory.pop_front();
            }

            if (serverState->wasRejected){

                if (clientStateHistory.size() > MAX_CLIENT_STATE_HISTORY) {
                    // Backlog is too large to replay without stalling the main thread (this happens
                    // when acks get delayed, e.g. by a burst of chunk traffic while moving fast).
                    // Snap to the server's authoritative position and drop the backlog instead of
                    // replaying every buffered step synchronously. Rotation is left alone since it's
                    // already kept live by the mouse callback, independent of this history.
                    std::cerr << "Warning: dropping " << clientStateHistory.size() << " unacknowledged inputs after large desync." << std::endl;
                    player->setPosition(serverState->truePosition);
                    clientStateHistory.clear();
                } else {
                    player->setPosition(serverState->truePosition);
                    player->setRotation(lastYaw, lastPitch);

                    for (auto& state : clientStateHistory) {
                        player->setRotation(state.yaw, state.pitch);
                        player->handleInput(state.input);
                        player->MouseInput(state.xoffset, state.yoffset);
                        player->updatePhysics(state.dt);
                        state.predictedPosition = player->getPosition();
                    }
                }

            }

            break;
        }
        case PacketType::BlockUpdate: {

            BlockUpdatePacket* blockUpdatePacket = reinterpret_cast<BlockUpdatePacket*>(event.packet->data);
            worldAccessor->updateBlock(blockUpdatePacket->position, blockUpdatePacket->block);
            break;

        }
        case PacketType::UnloadChunkColumn: {

            if (event.packet->dataLength != sizeof(UnloadChunkColumnPacket)) {
                std::cerr << "Warning: Invalid UnloadChunkColumnPacket length: " << event.packet->dataLength << std::endl;
                break;
            }
            UnloadChunkColumnPacket* unloadPacket = reinterpret_cast<UnloadChunkColumnPacket*>(event.packet->data);
            worldAccessor->unloadChunkColumn(glm::ivec2(unloadPacket->chunkColumnX, unloadPacket->chunkColumnZ));
            break;

        }
        default:
            std::cerr << "Unknown packet type received: " << static_cast<int>(packetType) << std::endl;
            break;
    }

}

