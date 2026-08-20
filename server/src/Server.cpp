#include "Server.hpp"
#include <DBG.hpp>

Server::Server() {
    if (enet_initialize() != 0) {
        throw std::runtime_error("Failed to initialize ENet");
    }

    address.host = ENET_HOST_ANY; // Bind to all available interfaces
    address.port = 1234; // Set the port number

    server = enet_host_create(&address, 32, 2, 0, 0); // Create the server host
    if (server == nullptr) {
        throw std::runtime_error("Failed to create ENet server host");
    }

    setRenderDistance(renderDistance); // Initialize chunksToLoadOffset based on the render distance

    std::cout << "Server initialized and listening on port " << address.port << std::endl;

}

void Server::setRenderDistance(int distance) {
    renderDistance = distance;
    std::vector<std::vector<bool>> visited(renderDistance * 2 + 1, std::vector<bool>(renderDistance * 2 + 1, false));
    chunksToLoadOffset.clear();

    std::queue<glm::ivec2> q;
    q.push({renderDistance, renderDistance});
    visited[renderDistance][renderDistance] = true;

    glm::ivec2 offsets[4] = {{1,0},{-1,0},{0,1},{0,-1}};

    while (!q.empty()) {
        glm::ivec2 pos = q.front();
        q.pop();
        int x = pos.x;
        int y = pos.y;
        if ((x - renderDistance) * (x - renderDistance) + (y - renderDistance) * (y - renderDistance) <= renderDistance * renderDistance) {
            chunksToLoadOffset.push_back({x - renderDistance, y - renderDistance});
            for (auto offset : offsets) {
                int nx = x + offset.x;
                int ny = y + offset.y;
                if (nx >= 0 && nx < visited.size() && ny >= 0 && ny < visited[0].size() && !visited[nx][ny]) {
                    visited[nx][ny] = true;
                    q.push({nx, ny});
                }
            }
        }

    }
}

Server::~Server() {
    enet_host_destroy(server);
    enet_deinitialize();    
}

void Server::run() {
    
    running = true;
    world.startTerrainGenerationThreads();

    while(running) {

        auto tickStartTime = std::chrono::high_resolution_clock::now();

#ifdef PERFORMANCE_DEBUG
        auto stepStartTime = tickStartTime;
        auto logStep = [&stepStartTime](const char* stepName) {
            auto now = std::chrono::high_resolution_clock::now();
            std::cout << "[PERF] " << stepName << ": "
                      << std::chrono::duration_cast<std::chrono::microseconds>(now - stepStartTime).count()
                      << "us" << std::endl;
            stepStartTime = now;
        };
#endif

        ENetEvent event;
        while (enet_host_service(server, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {

                    std::cout << "A new client connected from "
                                << event.peer->address.host << ":" << event.peer->address.port
                                << std::endl;
                    glm::vec3 initialPosition(0.0f, 80.0f, 0.0f);
                    connectedClients[event.peer->address] = {event.peer, initialPosition, glm::ivec2(0, 0), {}, 0};
                    newlyConnectedClients.insert(event.peer->address); // Mark the client as newly connected
                    event.peer->data = (void*)"Player 1";

                    //sent PlayerInitialPositionPacket to the client

                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE:
                    handlePacket(event);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT: {
                    std::cout << (char *)event.peer->data << " disconnected." << std::endl;
                    event.peer->data = nullptr;
                    // Unload all chunk columns for this player
                    std::vector<glm::ivec2> chunkColumnsToUnload;
                    auto &player = connectedClients[event.peer->address];
                    for (const auto& chunkColumnPos : player.loadedChunkColumns) {
                        chunkColumnsToUnload.push_back(chunkColumnPos);
                    }
                    world.unloadChunkColumns(chunkColumnsToUnload);
                    connectedClients.erase(event.peer->address);
                    break;
                }
                case ENET_EVENT_TYPE_NONE:
                    break;
            }
        }

#ifdef PERFORMANCE_DEBUG
        logStep("Event handling");
#endif

        // update chunks for players
        std::vector<glm::ivec2> chunkColumnsToUnload;
        for (auto& [address, player] : connectedClients) {
            //std::cout << "Player at position: (" << player.position.x << ", " << player.position.y << ", " << player.position.z << ")" << std::endl;
            glm::ivec2 currentChunkColumnPos = {static_cast<int>(std::floor(player.position.x / WorldConfig::CHUNK_SIZE)),
                                      static_cast<int>(std::floor(player.position.z / WorldConfig::CHUNK_SIZE))};
            
            // check if player has moved to a new chunk column
            bool isNewlyConnected = newlyConnectedClients.find(address) != newlyConnectedClients.end();
            if (isNewlyConnected) {
                newlyConnectedClients.erase(address); // Remove from newly connected set
            }
            if (currentChunkColumnPos != player.previousChunkColumnPosition || isNewlyConnected || 1) {

                // loading chunks based on render distance
                for (auto& offset : chunksToLoadOffset) {
                    glm::ivec2 chunkColumnToLoad = currentChunkColumnPos + offset;
                    if (player.loadedChunkColumns.find(chunkColumnToLoad) == player.loadedChunkColumns.end()) {
                        player.loadedChunkColumns.insert(chunkColumnToLoad);
                        world.playerLoadedChunkColumn(chunkColumnToLoad);
                        world.sentAndLoadChunkColumn(chunkColumnToLoad, player.peer);
                    }
                }

                // unloading chunks based on render distance
                // iterate back-to-front and erase by iterator: unordered_dense erases via swap-and-pop,
                // which invalidates end() and can skip elements under forward iteration + erase
                for (auto idx = player.loadedChunkColumns.size(); idx > 0;) {
                    --idx;
                    auto it = player.loadedChunkColumns.begin() + static_cast<std::ptrdiff_t>(idx);
                    glm::ivec2 chunkColumnPos = *it;
                    if ((currentChunkColumnPos.x - chunkColumnPos.x) * (currentChunkColumnPos.x - chunkColumnPos.x) +
                        (currentChunkColumnPos.y - chunkColumnPos.y) * (currentChunkColumnPos.y - chunkColumnPos.y) > renderDistance * renderDistance) {
                        chunkColumnsToUnload.push_back(chunkColumnPos);
                        player.loadedChunkColumns.erase(it);
                        sendUnloadChunkColumnPacket(player.peer, chunkColumnPos);
                    }
                }

            }
        }

#ifdef PERFORMANCE_DEBUG
        logStep("Chunk load/unload bookkeeping");
#endif

        world.unloadChunkColumns(chunkColumnsToUnload);
        //world.loadedChunkColumn();

#ifdef PERFORMANCE_DEBUG
        logStep("Chunk column unloading");
#endif

        // Chunk sending
        {
            std::lock_guard<std::mutex> lock(world.chunksToSendMutex);
            int chunksSentThisTick = 0;
            while (!world.chunksToSend.empty() && chunksSentThisTick < maxChunkColumnsSentPerTick) {
                ChunkToSend& chunkToSend = world.chunksToSend.front();
                if (world.playersLoadedChunkColumn(chunkToSend.chunkPacket->position) <= 0) {
                    world.chunksToSend.pop(); // Remove the chunk from the queue
                    continue; // Skip sending this chunk if no players have it loaded
                }
                if (chunkToSend.upload) {
                    world.uploadChunk(chunkToSend.chunkPacket);
                }
                ChunkSendBuffer* chunkColumnBuffers = world.chunkColumnToPacket(*chunkToSend.chunkPacket);
                for (int y = 0; y < WorldConfig::CHUNK_COLUMN_HEIGHT; y++) {
                    ENetPacket* packet = enet_packet_create(chunkColumnBuffers[y].data, chunkColumnBuffers[y].length, ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(chunkToSend.peer, 0, packet);
                }
                delete[] chunkColumnBuffers;
                //std::osyncstream(std::cout) << "Sent chunk at position: (" << chunkToSend.chunkPacket.position.x << ", " << chunkToSend.chunkPacket.position.y << ") to client." << std::endl;
                world.chunksToSend.pop();
                chunksSentThisTick++;
            }
        }

#ifdef PERFORMANCE_DEBUG
        logStep("Chunk sending");
#endif

        // input handling
        std::string command = "";
        {
            std::lock_guard<std::mutex> lock(commandMutex);
            if (!commandQueue.empty()) {
                command = commandQueue.front();
                commandQueue.pop();
            }
        }

        // command processing
        if (!command.empty()) {
            if (command == "exit") {
                std::cout << "Shutting down server..." << std::endl;
                running = false;
            } else {
                std::cout << "Unknown command: " << command << std::endl;
            }
        }

#ifdef PERFORMANCE_DEBUG
        logStep("Command handling");
#endif

        auto tickEndTime = std::chrono::high_resolution_clock::now();
        auto tickDuration = std::chrono::duration_cast<std::chrono::milliseconds>(tickEndTime - tickStartTime);
        if (tickDuration < TICK_DURATION) {
            std::this_thread::sleep_for(TICK_DURATION - tickDuration);
        } else {
            std::cout << "Warning: Server tick took longer than expected: " << tickDuration.count() << "ms" << std::endl;
        }

    }

    consoleThread.detach();
    world.stopTerrainGenerationThreads();

}

void Server::sendUnloadChunkColumnPacket(ENetPeer* peer, glm::ivec2 chunkColumnPos) {
    UnloadChunkColumnPacket packet;
    packet.chunkColumnX = chunkColumnPos.x;
    packet.chunkColumnZ = chunkColumnPos.y;
    ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(UnloadChunkColumnPacket), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, enetPacket);
}

void Server::handlePacket(ENetEvent& event) {
    
    PacketType packetType = static_cast<PacketType>(event.packet->data[0]);
    switch(packetType) {
        /* archaic not used anymore, but kept for reference
        case PacketType::RequestChunk: {
            
            RequestChunkPacket* requestChunkPacket = reinterpret_cast<RequestChunkPacket*>(event.packet->data);
            
            glm::vec3 playerPosition = connectedClients[event.peer->address].position;
            glm::ivec2 chunkColumnPos = {requestChunkPacket->chunkColumnX, requestChunkPacket->chunkColumnZ};
            glm::ivec2 playerChunkColumnPos = {static_cast<int>(std::floor(playerPosition.x / WorldConfig::CHUNK_SIZE)),
            static_cast<int>(std::floor(playerPosition.z / WorldConfig::CHUNK_SIZE))};
            if ((chunkColumnPos.x - playerChunkColumnPos.x) * (chunkColumnPos.x - playerChunkColumnPos.x) + 
            (chunkColumnPos.y - playerChunkColumnPos.y) * (chunkColumnPos.y - playerChunkColumnPos.y) > renderDistance * renderDistance) {
                //std::cout << "Client requested chunk column at position: (" << requestChunkPacket->chunkColumnX << ", " << requestChunkPacket->chunkColumnZ << ") which is outside of the render distance. Ignoring request." << std::endl;
                break;
            }
            
            std::lock_guard<std::mutex> lock(chunksToGenerateMutex);
            chunksToGenerate.push({requestChunkPacket->chunkColumnX, requestChunkPacket->chunkColumnZ, event.peer});
            chunksToGenerateCV.notify_one();
            
            break;
        }
        */
        case PacketType::PlayerMove: {

            PlayerMovePacket* playerMovePacket = reinterpret_cast<PlayerMovePacket*>(event.packet->data);
            auto it = connectedClients.find(event.peer->address);
            if (it != connectedClients.end()) {
                Player& player = it->second;
                if (playerMovePacket->sequenceID > player.lastProcessedInputSequenceID){

                    player.previousChunkColumnPosition = {static_cast<int>(std::floor(player.position.x / WorldConfig::CHUNK_SIZE)),
                                                          static_cast<int>(std::floor(player.position.z / WorldConfig::CHUNK_SIZE))};

                    Entity entity;
                    entity.position = player.position;
                    entity.velocity = playerMovePacket->position - player.position;
                    setBoundingBoxForPlayer(entity);

                    glm::vec3 deltaMovmemnt = updateEntity(entity, 1.0f, world);
                    player.position += deltaMovmemnt;
                    
                    ServerAuthoritativeStatePacket serverAuthoritativeStatePacket;
                    serverAuthoritativeStatePacket.lastProcessedInputSequenceID = playerMovePacket->sequenceID;
                    serverAuthoritativeStatePacket.truePosition = player.position;

                    // send server authoritative state packet
                    player.lastProcessedInputSequenceID = playerMovePacket->sequenceID;
                    if (abs(player.position.x - playerMovePacket->position.x) > 0.001f || 
                        abs(player.position.y - playerMovePacket->position.y) > 0.001f || 
                        abs(player.position.z - playerMovePacket->position.z) > 0.001f) {
                        serverAuthoritativeStatePacket.wasRejected = true;
                        
                        // IMPORTANT: Use ENET_PACKET_FLAG_RELIABLE to ensure the packet is delivered reliably
                        ENetPacket* packet = enet_packet_create(&serverAuthoritativeStatePacket, sizeof(ServerAuthoritativeStatePacket), ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 0, packet);
                    } else {
                        serverAuthoritativeStatePacket.wasRejected = false;

                        // not IMPORTANT: send on channel 1 without ENET_PACKET_FLAG_RELIABLE since it's not critical to be delivered
                        ENetPacket* packet = enet_packet_create(&serverAuthoritativeStatePacket, sizeof(ServerAuthoritativeStatePacket), 0);
                        enet_peer_send(event.peer, 1, packet);
                    }

                }
            }
            break;

        }
        case PacketType::PlaceBlock: {
            
            PlaceBlockPacket* placeBlockPacket = reinterpret_cast<PlaceBlockPacket*>(event.packet->data);
            world.updateBlock(placeBlockPacket->position, placeBlockPacket->block);

            BlockUpdatePacket blockUpdatePacket;
            blockUpdatePacket.position = placeBlockPacket->position;
            blockUpdatePacket.block = placeBlockPacket->block;

            ENetPacket* packet = enet_packet_create(&blockUpdatePacket, sizeof(BlockUpdatePacket), ENET_PACKET_FLAG_RELIABLE);
            for (int i = 0; i < server->peerCount; i++) {
                ENetPeer* peer = &server->peers[i];
                if (peer->state != ENET_PEER_STATE_CONNECTED || peer == event.peer) {
                    continue;
                }
                enet_peer_send(peer, 0, packet);
            }

            break;

        }
        case PacketType::DestroyBlock: {

            DestroyBlockPacket* destroyBlockPacket = reinterpret_cast<DestroyBlockPacket*>(event.packet->data);
            world.updateBlock(destroyBlockPacket->position, Block::AIR);

            BlockUpdatePacket blockUpdatePacket;
            blockUpdatePacket.position = destroyBlockPacket->position;
            blockUpdatePacket.block = Block::AIR;

            ENetPacket* packet = enet_packet_create(&blockUpdatePacket, sizeof(BlockUpdatePacket), ENET_PACKET_FLAG_RELIABLE);
            for (int i = 0; i < server->peerCount; i++) {
                ENetPeer* peer = &server->peers[i];
                if (peer->state != ENET_PEER_STATE_CONNECTED || peer == event.peer) {
                    continue;
                }
                enet_peer_send(peer, 0, packet);
            }

            break;

        }
        default:
        
            break;
    }

    enet_packet_destroy(event.packet);
}

void Server::consoleInput() {

    std::string line;
    while (running && std::getline(std::cin, line)) {
        if (!line.empty()) {
            std::lock_guard<std::mutex> lock(commandMutex);
            commandQueue.push(line);
        }
    }

}