#include "Server.hpp"

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

    std::cout << "Server initialized and listening on port " << address.port << std::endl;

}

void Server::setRenderDistance(int distance) {
    renderDistance = distance;
    renderDistanceServer = renderDistance + 1;
    std::vector<std::vector<bool>> visited(renderDistanceServer * 2 + 1, std::vector<bool>(renderDistanceServer * 2 + 1, false));
    chunksToLoadOffset.clear();

    std::queue<glm::ivec2> q;
    q.push({renderDistanceServer, renderDistanceServer});
    visited[renderDistanceServer][renderDistanceServer] = true;

    glm::ivec2 offsets[4] = {{1,0},{-1,0},{0,1},{0,-1}};

    while (!q.empty()) {
        glm::ivec2 pos = q.front();
        q.pop();
        int x = pos.x;
        int y = pos.y;
        if ((x - renderDistanceServer) * (x - renderDistanceServer) + (y - renderDistanceServer) * (y - renderDistanceServer) <= renderDistanceServer * renderDistanceServer) {
            chunksToLoadOffset.push_back({x - renderDistanceServer, y - renderDistanceServer});
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
    consoleThread = std::thread(&Server::consoleInput, this);
    for (int i = 0; i < numChunkGenerationThreads; ++i) {
        chunkGenerationThreads.emplace_back(&Server::generateChunks, this, i);
    }

    while(running) {

        auto tickStartTime = std::chrono::high_resolution_clock::now();

        ENetEvent event;
        while (enet_host_service(server, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {

                    std::cout << "A new client connected from "
                                << event.peer->address.host << ":" << event.peer->address.port
                                << std::endl;
                    glm::vec3 initialPosition(0.0f, 80.0f, 0.0f);
                    connectedClients[event.peer->address] = {event.peer, initialPosition, glm::ivec2(0, 0), {}, 0};
                    event.peer->data = (void*)"Player 1";

                    //sent PlayerInitialPositionPacket to the client

                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE:
                    handlePacket(event);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    std::cout << (char *)event.peer->data << " disconnected." << std::endl;
                    event.peer->data = nullptr;
                    break;
                case ENET_EVENT_TYPE_NONE:
                    break;
            }
        }

        // requesting chunks for player implement later
        /*
        for (auto& [address, player] : connectedClients) {
            
            glm::ivec2 currentChunkColumnPos = {static_cast<int>(std::floor(player.position.x / WorldConfig::CHUNK_SIZE)),
                                      static_cast<int>(std::floor(player.position.z / WorldConfig::CHUNK_SIZE))};
            
            // check if player has moved to a new chunk column
            if (currentChunkColumnPos != player.previousChunkColumnPosition) {
                // check if the player has already been sent the chunk column
                for (auto& chunkColumnSent : player.sentChunkColumns) {
                    if ((chunkColumnSent.x - currentChunkColumnPos.x) * (chunkColumnSent.x - currentChunkColumnPos.x) + 
                        (chunkColumnSent.y - currentChunkColumnPos.y) * (chunkColumnSent.y - currentChunkColumnPos.y) > renderDistanceServer * renderDistanceServer) {
                        player.sentChunkColumns.erase(chunkColumnSent);
                    }
                }
                for (auto& offset : chunksToLoadOffset) {
                    glm::ivec2 chunkColumnToLoad = currentChunkColumnPos + offset;
                    if (player.sentChunkColumns.find(chunkColumnToLoad) == player.sentChunkColumns.end()) {
                        // send request for chunk column to the client
                        RequestChunkPacket requestChunkPacket;
                        requestChunkPacket.chunkColumnX = chunkColumnToLoad.x;
                        requestChunkPacket.chunkColumnZ = chunkColumnToLoad.y;
                        //Load the chunk column from the world
                    }
                }
            }

        }
        */
        // Chunk sending
        {
            std::lock_guard<std::mutex> lock(chunksToSendMutex);
            while (!chunksToSend.empty()) {
                ChunkToSend& chunkToSend = chunksToSend.front();
                ENetPacket* packet = enet_packet_create(&chunkToSend.chunkPacket, sizeof(ChunkPacket), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(chunkToSend.peer, 0, packet);
                //std::osyncstream(std::cout) << "Sent chunk at position: (" << chunkToSend.chunkPacket.x << ", " << chunkToSend.chunkPacket.y << ", " << chunkToSend.chunkPacket.z << ") to client." << std::endl;
                chunksToSend.pop();
            }
        }

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
                chunksToGenerateCV.notify_all(); // Notify the chunk generation thread to exit
            } else {
                std::cout << "Unknown command: " << command << std::endl;
            }
        }

        auto tickEndTime = std::chrono::high_resolution_clock::now();
        auto tickDuration = std::chrono::duration_cast<std::chrono::milliseconds>(tickEndTime - tickStartTime);
        if (tickDuration < TICK_DURATION) {
            std::this_thread::sleep_for(TICK_DURATION - tickDuration);
        } else {
            std::cout << "Warning: Server tick took longer than expected: " << tickDuration.count() << "ms" << std::endl;
        }

    }

    consoleThread.detach();
    for (auto& thread : chunkGenerationThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

}

void Server::handlePacket(ENetEvent& event) {
    
    PacketType packetType = static_cast<PacketType>(event.packet->data[0]);
    switch(packetType) {
        // /* archaic not used anymore, but kept for reference
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

void Server::generateChunks(int threadId) {

    std::osyncstream(std::cout) << "Chunk generation thread " << threadId+1 << " started." << std::endl;

    int chunksGenerated = 0;
    while (running) {

        ChunkToGenerate chunkToGenerate[chunkToGeneratePerThread];
        int chunksToGenerateCount = 0;

        {
            std::unique_lock<std::mutex> lock(chunksToGenerateMutex);

            chunksToGenerateCV.wait(lock, [this] { 
                return !chunksToGenerate.empty() || !running; 
            });

            if (!running){
                break;
            }

            while (chunksToGenerateCount < chunkToGeneratePerThread && !chunksToGenerate.empty()) {
                chunkToGenerate[chunksToGenerateCount] = chunksToGenerate.front();
                chunksToGenerate.pop();
                chunksToGenerateCount++;
            }
        }


        ChunkPacket chunkPackets[chunkToGeneratePerThread][WorldConfig::CHUNK_COLUMN_HEIGHT];
        for (int i = 0; i < chunksToGenerateCount; i++){
            world.getTerrain(glm::ivec2(chunkToGenerate[i].x, chunkToGenerate[i].z), chunkPackets[i]);
        }

        {
            std::lock_guard<std::mutex> lock(chunksToSendMutex);
            for (int i = 0; i < chunksToGenerateCount; i++) {
                for (int j = 0; j < WorldConfig::CHUNK_COLUMN_HEIGHT; j++) {
                    chunksToSend.push({chunkPackets[i][j], chunkToGenerate[i].peer});
                }
            }
            chunksGenerated += WorldConfig::CHUNK_COLUMN_HEIGHT * chunksToGenerateCount;
        }

    }

    std::osyncstream(std::cout) << "Chunk generation thread " << threadId+1 << " exiting. Total chunks generated: " << chunksGenerated << std::endl;

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