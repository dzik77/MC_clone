#include <WorldGeneration/World.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <stdexcept>

const std::vector<std::filesystem::path> texturePaths = {
    "textures/dirt.png",
    "textures/grass_block_side.png",
    "textures/grass_block_top_item.png",
    "textures/stone.png",
    "textures/water_still.png"
};


World::World() {



}

void World::initialize() {

    vramAllocator.initialize(140'000'000, 250'000'000);
    
    glCreateVertexArrays(1, &globalVAO);
    
    glVertexArrayElementBuffer(globalVAO, vramAllocator.getIndexBufferID());
    
    glEnableVertexArrayAttrib(globalVAO, 0);
    glVertexArrayAttribIFormat(globalVAO, 0, 1, GL_UNSIGNED_INT, 0);
    
    glVertexArrayAttribBinding(globalVAO, 0, 0);
    glVertexArrayVertexBuffer(globalVAO, 0, vramAllocator.getVertexBufferID(), 0, sizeof(uint32_t));
    
    
    std::cout << "Loading textures..." << std::endl;
    loadTextures();
    std::cout << "Textures loaded." << std::endl; 
    
    glCreateBuffers(1, &chunkSSBO);
    glNamedBufferStorage(chunkSSBO, PREALLOCATE_CHUNKS * sizeof(ChunkPosition), nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, chunkSSBO);
    ssboData = (ChunkPosition*)glMapNamedBufferRange(chunkSSBO, 0, PREALLOCATE_CHUNKS * sizeof(ChunkPosition), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    
    indirectCommandBufferSize = PREALLOCATE_CHUNKS * sizeof(DrawElementsIndirectCommand) * 4;
    glCreateBuffers(1, &indirectCommandBuffer);
    glNamedBufferStorage(indirectCommandBuffer, indirectCommandBufferSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    mappedIndirectCommands = (DrawElementsIndirectCommand*)glMapNamedBufferRange(indirectCommandBuffer, 0, indirectCommandBufferSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    
    for (int i = 0; i < PREALLOCATE_CHUNKS; i++){
        freeChunkPositionIndices.push(i);
    }

    setRenderDistance(renderDistance);

}

void World::setRenderDistance(int distance) {

    renderDistance = distance;
    std::vector<std::vector<bool>> visited(renderDistance * 2 + 1, std::vector<bool>(renderDistance * 2 + 1, false));
    chunksToLoadOffset.clear();

    std::queue<std::pair<int,int>> q;
    q.push({renderDistance, renderDistance});
    visited[renderDistance][renderDistance] = true;

    std::pair<int,int> offsets[4] = {{1,0},{-1,0},{0,1},{0,-1}};

    while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        if ((x - renderDistance) * (x - renderDistance) + (y - renderDistance) * (y - renderDistance) <= renderDistance * renderDistance) {
            chunksToLoadOffset.push_back({x - renderDistance, y - renderDistance});
            for (auto [dx, dy] : offsets) {
                int nx = x + dx;
                int ny = y + dy;
                if (nx >= 0 && nx < visited.size() && ny >= 0 && ny < visited[0].size() && !visited[nx][ny]) {
                    visited[nx][ny] = true;
                    q.push({nx, ny});
                }
            }
        }

    }

}

void World::loadTextures() {
    textureArray.createTextureArray(texturePaths,16,16);
}

void World::generateTerrain(ChunkColumn &chunkColumn) {

    // load from server TODO:
    // i guess for now just auto wait for the chunk

    //std::cout << "Requesting chunk column at position: (" << chunkColumn.position.x << ", " << chunkColumn.position.y << ")" << std::endl;

    // request chunk from server
    RequestChunkPacket requestPacket;
    requestPacket.chunkColumnX = chunkColumn.position.x;
    requestPacket.chunkColumnZ = chunkColumn.position.y;
    ENetPacket *packet = enet_packet_create(&requestPacket, sizeof(RequestChunkPacket), ENET_PACKET_FLAG_RELIABLE);
    assert(serverPeer != nullptr);
    enet_peer_send(serverPeer, 0, packet);

    // for now just wait for the chunk to be received
    /*
    ENetEvent event;
    int chunksReceived = 0;
    while (chunksReceived < WorldConfig::CHUNK_COLUMN_HEIGHT) {
        while (enet_host_service(serverPeer->host, &event, 50) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                PacketType packetType = static_cast<PacketType>(event.packet->data[0]);
                if (packetType == PacketType::Chunk) {
                    ChunkPacket* chunkPacket = reinterpret_cast<ChunkPacket*>(event.packet->data);
                    
                    if (chunkPacket->x != chunkColumn.position.x || chunkPacket->z != chunkColumn.position.y) {
                        std::cerr << "Received chunk for wrong position: (" << chunkPacket->x << ", " << chunkPacket->z << "), expected: (" << chunkColumn.position.x << ", " << chunkColumn.position.y << ")" << std::endl;
                        enet_packet_destroy(event.packet);
                        continue;
                    }
                    
                    for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++) {
                        for (int y = 0; y < WorldConfig::CHUNK_SIZE; y++) {
                            for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++) {
                                chunkColumn.blocks[x][y + chunkPacket->y * WorldConfig::CHUNK_SIZE][z] = chunkPacket->chunkBlocks[x * WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_SIZE + y * WorldConfig::CHUNK_SIZE + z];
                            }
                        }
                    }
                    chunksReceived++;
                }
                enet_packet_destroy(event.packet);
            }
        }
    }
    
    std::cout << "Received chunk column at position: (" << chunkColumn.position.x << ", " << chunkColumn.position.y << ")" << std::endl;
    */
    
    // Assign SSBO indices to the chunks in the column
    for (int i = 0; i < WorldConfig::CHUNK_COLUMN_HEIGHT; i++){
        if (!freeChunkPositionIndices.empty()){
            chunkColumn.chunks[i].ssboIndex = freeChunkPositionIndices.top();
            freeChunkPositionIndices.pop();
            //std::cout << "Assigned SSBO index " << chunkColumn.chunks[i].ssboIndex << " to chunk at height " << i << std::endl;
            //std::cout << "Chunk coords: " << chunkColumn.position.x << ", " << chunkColumn.position.y << ", " << i << std::endl;
            ssboData[chunkColumn.chunks[i].ssboIndex].position = glm::vec4(chunkColumn.position.x * WorldConfig::CHUNK_SIZE, i * WorldConfig::CHUNK_SIZE, chunkColumn.position.y * WorldConfig::CHUNK_SIZE, 0.0f);
        } else {
            throw std::runtime_error("Not enough SSBO indices! Increase PREALLOCATE_CHUNKS.");
        }
    }
    //std::cout << "Terrain generated for chunk column at position: " << chunkColumn.position.x << ", " << chunkColumn.position.y << std::endl;


}

void World::loadTerrain() {
    
    while (!receivedChunks.empty()) {

        auto chunkPacket = receivedChunks.front();
        receivedChunks.pop();

        std::pair<int,int> chunkColumnPos = {chunkPacket.x, chunkPacket.z};
        auto it = loadedChunkPositions.find(chunkColumnPos);
    
        if (it == loadedChunkPositions.end()){
            return; // chunk column not loaded, ignore
        }
    
        for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++) {
            for (int y = 0; y < WorldConfig::CHUNK_SIZE; y++) {
                for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++) {
                    ChunkColumns[it->second].blocks[x][y + chunkPacket.y * WorldConfig::CHUNK_SIZE][z] = chunkPacket.chunkBlocks[x * WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_SIZE + y * WorldConfig::CHUNK_SIZE + z];
                }
            }
        }
        uploadedChunkPositions[chunkColumnPos] += 1;
        if (uploadedChunkPositions[chunkColumnPos] == WorldConfig::CHUNK_COLUMN_HEIGHT){
            chunksToGenerateMesh.push(it->second);
        }

    }


}

void World::generateMesh() {

    while(!chunksGeneratedOnEdge.empty()){
        int idx = *chunksGeneratedOnEdge.begin();
        auto& chunkColumn = ChunkColumns[idx];
        chunksGeneratedOnEdge.erase(chunksGeneratedOnEdge.begin());
        Block (*blocks_Front)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        Block (*blocks_Back)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        Block (*blocks_Left)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        Block (*blocks_Right)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        auto it = loadedChunkPositions.find({chunkColumn.position.x-1, chunkColumn.position.y});
        if (it != loadedChunkPositions.end()){
            blocks_Left = ChunkColumns[it->second].blocks;
        }
        it = loadedChunkPositions.find({chunkColumn.position.x+1, chunkColumn.position.y});
        if (it != loadedChunkPositions.end()){
            blocks_Right = ChunkColumns[it->second].blocks;
        }
        it = loadedChunkPositions.find({chunkColumn.position.x, chunkColumn.position.y+1});
        if (it != loadedChunkPositions.end()){
            blocks_Front = ChunkColumns[it->second].blocks;
        }
        it = loadedChunkPositions.find({chunkColumn.position.x, chunkColumn.position.y-1});
        if (it != loadedChunkPositions.end()){
            blocks_Back = ChunkColumns[it->second].blocks;
        }
        for (int height = 0; height < WorldConfig::CHUNK_COLUMN_HEIGHT; height++){
            chunkColumn.chunks[height].generateMesh(
                chunkColumn.blocks, height,
                blocks_Front,
                blocks_Back,
                blocks_Left,
                blocks_Right
            );
        }
    }
    while(!chunksToGenerateMesh.empty()){
        int idx = chunksToGenerateMesh.top();
        auto& chunkColumn = ChunkColumns[idx];
        chunksToGenerateMesh.pop();
        Block (*blocks_Front)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        Block (*blocks_Back)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        Block (*blocks_Left)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        Block (*blocks_Right)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        auto it = loadedChunkPositions.find({chunkColumn.position.x-1, chunkColumn.position.y});
        if (it != loadedChunkPositions.end()){
            blocks_Left = ChunkColumns[it->second].blocks;
            if (chunksGeneratedOnEdge.find(it->second) == chunksGeneratedOnEdge.end()){
                chunksGeneratedOnEdge.insert(it->second);
            }
        }
        it = loadedChunkPositions.find({chunkColumn.position.x+1, chunkColumn.position.y});
        if (it != loadedChunkPositions.end()){
            blocks_Right = ChunkColumns[it->second].blocks;
            if (chunksGeneratedOnEdge.find(it->second) == chunksGeneratedOnEdge.end()){
                chunksGeneratedOnEdge.insert(it->second);
            }
        }
        it = loadedChunkPositions.find({chunkColumn.position.x, chunkColumn.position.y+1});
        if (it != loadedChunkPositions.end()){
            blocks_Front = ChunkColumns[it->second].blocks;
            if (chunksGeneratedOnEdge.find(it->second) == chunksGeneratedOnEdge.end()){
                chunksGeneratedOnEdge.insert(it->second);
            }
        }
        it = loadedChunkPositions.find({chunkColumn.position.x, chunkColumn.position.y-1});
        if (it != loadedChunkPositions.end()){
            blocks_Back = ChunkColumns[it->second].blocks;
            if (chunksGeneratedOnEdge.find(it->second) == chunksGeneratedOnEdge.end()){
                chunksGeneratedOnEdge.insert(it->second);
            }
        }
        for (int height = 0; height < WorldConfig::CHUNK_COLUMN_HEIGHT; height++){
            chunkColumn.chunks[height].generateMesh(
                chunkColumn.blocks, height,
                blocks_Front,
                blocks_Back,
                blocks_Left,
                blocks_Right
            );
        }
    }

}

void World::loadChunks(glm::vec3 playerPosition) {
    
    //std::cout << "Loading chunks around player position: " << playerPosition.x << ", " << playerPosition.y << ", " << playerPosition.z << std::endl;
    glm::ivec3 chunkPosition(
        static_cast<int>(std::floor(playerPosition.x / WorldConfig::CHUNK_SIZE)),
        0,
        static_cast<int>(std::floor(playerPosition.z / WorldConfig::CHUNK_SIZE))
    );
    int pt2 = 0;
    std::vector<int> toRemove;
    for (int i = 0; i < ChunkColumns.size(); i++){
        if ( (ChunkColumns[i].position.x-chunkPosition.x) * (ChunkColumns[i].position.x-chunkPosition.x) + (ChunkColumns[i].position.y-chunkPosition.z) * (ChunkColumns[i].position.y-chunkPosition.z) > renderDistance*renderDistance){
            toRemove.push_back(i);
        }
    }

    sortedChunkColumnsIndices.clear();
    for (auto [x,z] : chunksToLoadOffset){
        if (loadedChunkPositions.find(std::pair<int,int>(chunkPosition.x + x, chunkPosition.z + z)) == loadedChunkPositions.end()){
            if (toRemove.size() > 0){
                int idx = toRemove.back();
                toRemove.pop_back();
                for (int j = 0; j < WorldConfig::CHUNK_COLUMN_HEIGHT; j++) {
                    freeChunkPositionIndices.push(ChunkColumns[idx].chunks[j].ssboIndex);
                }
                loadedChunkPositions.erase(std::pair<int,int>(ChunkColumns[idx].position.x, ChunkColumns[idx].position.y));
                uploadedChunkPositions.erase(std::pair<int,int>(ChunkColumns[idx].position.x, ChunkColumns[idx].position.y));
                ChunkColumns[idx] = ChunkColumn(glm::vec2(chunkPosition.x + x, chunkPosition.z + z), &vramAllocator);
                generateTerrain(ChunkColumns[idx]);
                loadedChunkPositions[std::pair<int,int>(chunkPosition.x + x, chunkPosition.z + z)] = idx;
                sortedChunkColumnsIndices.push_back(idx);
            } else {
                ChunkColumns.emplace_back(glm::vec2(chunkPosition.x + x, chunkPosition.z + z), &vramAllocator);
                generateTerrain(ChunkColumns.back());
                loadedChunkPositions[std::pair<int,int>(chunkPosition.x + x, chunkPosition.z + z)] = ChunkColumns.size()-1;
                sortedChunkColumnsIndices.push_back(ChunkColumns.size()-1);
            }
        } else {
            sortedChunkColumnsIndices.push_back(loadedChunkPositions[std::pair<int,int>(chunkPosition.x + x, chunkPosition.z + z)]);
        }
    }
    
    for (int i = (int)toRemove.size() - 1; i >= 0; i--){
        int idx = toRemove[i];
        for (int j = 0; j < WorldConfig::CHUNK_COLUMN_HEIGHT; j++) {
            if (ChunkColumns[idx].chunks[j].ssboIndex != -1) {
                freeChunkPositionIndices.push(ChunkColumns[idx].chunks[j].ssboIndex);
                ChunkColumns[idx].chunks[j].ssboIndex = -1;
            }
        }
        loadedChunkPositions.erase(std::pair<int,int>(ChunkColumns[idx].position.x, ChunkColumns[idx].position.y));
        uploadedChunkPositions.erase(std::pair<int,int>(ChunkColumns[idx].position.x, ChunkColumns[idx].position.y));
        
        if (idx != ChunkColumns.size() - 1) {
            int lastIdx = ChunkColumns.size() - 1;
            ChunkColumns[idx] = std::move(ChunkColumns[lastIdx]);
            loadedChunkPositions[std::pair<int,int>(ChunkColumns[idx].position.x, ChunkColumns[idx].position.y)] = idx;
        }
        ChunkColumns.pop_back();
    }
    //std::cout << "Chunks loaded around player position: " << playerPosition.x << ", " << playerPosition.y << ", " << playerPosition.z << std::endl;
}

void World::render(Shader& shader, glm::vec3 playerPosition) {

    static bool firstRender = true;

    std::vector<DrawElementsIndirectCommand> drawCommandsOpaque;
    for (auto it = sortedChunkColumnsIndices.begin(); it != sortedChunkColumnsIndices.end(); ++it){
        auto& chunkColumn = ChunkColumns[*it];
        for (int i = 0; i < WorldConfig::CHUNK_COLUMN_HEIGHT; i++){
            if (chunkColumn.chunks[i].indiciesCountOpaque == 0 || chunkColumn.chunks[i].ssboIndex == -1) continue;
            DrawElementsIndirectCommand cmd;
            cmd.count = chunkColumn.chunks[i].indiciesCountOpaque;
            cmd.instanceCount = 1;
            cmd.firstIndex = chunkColumn.chunks[i].chunkAllocationOpaque.indexBlocks;
            cmd.baseVertex = chunkColumn.chunks[i].chunkAllocationOpaque.vertexBlocks;
            cmd.baseInstance = chunkColumn.chunks[i].ssboIndex;
            drawCommandsOpaque.push_back(cmd);
        }
    }

    std::vector<DrawElementsIndirectCommand> drawCommandsTransparent;
    for (auto it = sortedChunkColumnsIndices.rbegin(); it != sortedChunkColumnsIndices.rend(); ++it){
        auto& chunkColumn = ChunkColumns[*it];
        for (int j = 0; j < WorldConfig::CHUNK_COLUMN_HEIGHT; j++){
            if (chunkColumn.chunks[j].indiciesCountTransparent == 0 || chunkColumn.chunks[j].ssboIndex == -1) continue;
            DrawElementsIndirectCommand cmd;
            cmd.count = chunkColumn.chunks[j].indiciesCountTransparent;
            cmd.instanceCount = 1;
            cmd.firstIndex = chunkColumn.chunks[j].chunkAllocationTransparent.indexBlocks;
            cmd.baseVertex = chunkColumn.chunks[j].chunkAllocationTransparent.vertexBlocks;
            cmd.baseInstance = chunkColumn.chunks[j].ssboIndex;
            drawCommandsTransparent.push_back(cmd);
        }
    }

    static int currentFrame = 0;
    int bufferIndex = currentFrame % 2;
    currentFrame++;

    if (!drawCommandsOpaque.empty()) {
        DrawElementsIndirectCommand* cmdPtrOpaque = mappedIndirectCommands + bufferIndex * PREALLOCATE_CHUNKS;
        std::memcpy(cmdPtrOpaque, drawCommandsOpaque.data(), drawCommandsOpaque.size() * sizeof(DrawElementsIndirectCommand));
    }

    if (!drawCommandsTransparent.empty()) {
        DrawElementsIndirectCommand* cmdPtrTransparent = mappedIndirectCommands + (2 + bufferIndex) * PREALLOCATE_CHUNKS;
        std::memcpy(cmdPtrTransparent, drawCommandsTransparent.data(), drawCommandsTransparent.size() * sizeof(DrawElementsIndirectCommand));
    }

    shader.use();
    glBindTextureUnit(0, textureArray.texture);
    glBindVertexArray(globalVAO);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectCommandBuffer);

    // OPAQUE PASS
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    if (!drawCommandsOpaque.empty()) {
        glMultiDrawElementsIndirect(
            GL_TRIANGLES, 
            GL_UNSIGNED_SHORT, 
            (void *)(bufferIndex * PREALLOCATE_CHUNKS * sizeof(DrawElementsIndirectCommand)), 
            drawCommandsOpaque.size(), 
            0
        );
    }

    // TRANSPARENT PASS
    if (!drawCommandsTransparent.empty()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glMultiDrawElementsIndirect(
            GL_TRIANGLES, 
            GL_UNSIGNED_SHORT, 
            (void *)((2 + bufferIndex) * PREALLOCATE_CHUNKS * sizeof(DrawElementsIndirectCommand)), 
            drawCommandsTransparent.size(), 
            0
        );
    }

    // Restore state for next frame (critical for glClear(GL_DEPTH_BUFFER_BIT) in main loop)
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

Block World::getBlockAt(const glm::ivec3& position) const {

    int chunkColumnX = std::floor(static_cast<float>(position.x) / WorldConfig::CHUNK_SIZE);
    int chunkColumnZ = std::floor(static_cast<float>(position.z) / WorldConfig::CHUNK_SIZE);
    auto it = loadedChunkPositions.find({chunkColumnX, chunkColumnZ});
    if (it == loadedChunkPositions.end()) {
        return Block::AIR; // Chunk column not loaded
    }
    if (position.y < 0 || position.y >= WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT) {
        return Block::AIR; // Y position out of bounds
    }
    int chunkPosX = position.x % WorldConfig::CHUNK_SIZE;
    int chunkPosZ = position.z % WorldConfig::CHUNK_SIZE;   
    return ChunkColumns[it->second].blocks[chunkPosX][position.y][chunkPosZ];

}

void World::clean(){
    ChunkColumns.clear();
    textureArray.clean();
}

World::~World() {
    ChunkColumns.clear();
}
