#include <Engine/Networking.hpp>
#include <Engine/Player.hpp>

Networking::Networking() : client(nullptr), serverPeer(nullptr), receivedChunks(nullptr), player(nullptr), worldAccessor(nullptr) {}

Networking::~Networking() {
    if (client) {
        enet_host_destroy(client);
    }
}

void Networking::initialize(std::queue<ChunkPacket> &receivedChunks, Player &player, IWorldAccessor &worldAccessor) {
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

    clientStateHistory.push_back(state);

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
            if (event.packet->dataLength == sizeof(ChunkPacket)) {
                receivedChunks->push(*reinterpret_cast<ChunkPacket*>(event.packet->data));
            } else {
                std::cerr << "Warning: Invalid ChunkPacket length: " << event.packet->dataLength << std::endl;
            }
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

            break;
        }
        default:
            std::cerr << "Unknown packet type received: " << static_cast<int>(packetType) << std::endl;
            break;
    }

}

