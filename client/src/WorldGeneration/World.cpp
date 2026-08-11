#include <WorldGeneration/World.hpp>
#include <imgui.h>
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

void World::loadTextures() {
    textureArray.createTextureArray(texturePaths,16,16);
}

void World::generateTerrain(ChunkColumn &chunkColumn) {

#ifdef DEBUG
    chunkColumn.debugState = ChunkColumn::ChunkDebugState::Requested;
#endif

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

void World::updateBlock(glm::ivec3 position, Block block) {

    glm::ivec2 chunkColumnPos = {
        static_cast<int>(std::floor(static_cast<float>(position.x) / WorldConfig::CHUNK_SIZE)), 
        static_cast<int>(std::floor(static_cast<float>(position.z) / WorldConfig::CHUNK_SIZE))
    };
    auto it = chunkColumn.find(chunkColumnPos);
    if (it == chunkColumn.end()){
        return; // chunk column not loaded, ignore
    }
    if (position.y < 0 || position.y >= WorldConfig::CHUNK_COLUMN_HEIGHT * WorldConfig::CHUNK_SIZE) {
        return; // out of bounds, ignore
    }
    int chunkPosX = position.x - chunkColumnPos.x * WorldConfig::CHUNK_SIZE;
    int chunkPosZ = position.z - chunkColumnPos.y * WorldConfig::CHUNK_SIZE;
    if (it->second->blocks[chunkPosX][position.y][chunkPosZ] == block){
        return; // no change, ignore
    }
    it->second->blocks[chunkPosX][position.y][chunkPosZ] = block;
    
    glm::ivec3 chunkPos = {
        chunkColumnPos.x,
        position.y / WorldConfig::CHUNK_SIZE,
        chunkColumnPos.y
    };
    regenerateMeshForChunk(chunkPos);
    if (chunkPos.x % WorldConfig::CHUNK_SIZE == 0){
        regenerateMeshForChunk({chunkPos.x-1, chunkPos.y, chunkPos.z});
    } else if (chunkPos.x % WorldConfig::CHUNK_SIZE == WorldConfig::CHUNK_SIZE-1){
        regenerateMeshForChunk({chunkPos.x+1, chunkPos.y, chunkPos.z});
    }
    if (chunkPos.z % WorldConfig::CHUNK_SIZE == 0){
        regenerateMeshForChunk({chunkPos.x, chunkPos.y, chunkPos.z-1});
    } else if (chunkPos.z % WorldConfig::CHUNK_SIZE == WorldConfig::CHUNK_SIZE-1){
        regenerateMeshForChunk({chunkPos.x, chunkPos.y, chunkPos.z+1});
    }
    if (chunkPos.y % WorldConfig::CHUNK_SIZE == 0){
        regenerateMeshForChunk({chunkPos.x, chunkPos.y+1, chunkPos.z});
    } else if (chunkPos.y % WorldConfig::CHUNK_SIZE == WorldConfig::CHUNK_SIZE-1){
        regenerateMeshForChunk({chunkPos.x, chunkPos.y-1, chunkPos.z});
    }

}

void World::regenerateMeshForChunk(glm::ivec3 position) {
    
    std::cout << "Regenerating mesh for chunk at position: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;

    glm::ivec2 chunkColumnPos;
    chunkColumnPos.x = position.x;
    chunkColumnPos.y = position.z;
    auto it = chunkColumn.find(chunkColumnPos);
    if (it == chunkColumn.end()){
        return; // chunk column not loaded, ignore
    }
    if (position.y < 0 || position.y >= WorldConfig::CHUNK_COLUMN_HEIGHT) {
        return; // out of bounds, ignore
    }

    // add to mesh queue for priority meshing
    std::shared_ptr<ChunkColumn> 
        chunkColumFront(nullptr), 
        chunkColumBack(nullptr), 
        chunkColumLeft(nullptr), 
        chunkColumRight(nullptr);
    auto itFront = chunkColumn.find({chunkColumnPos.x, chunkColumnPos.y+1});
    if (itFront != chunkColumn.end()){
        chunkColumFront = itFront->second;
    }
    auto itBack = chunkColumn.find({chunkColumnPos.x, chunkColumnPos.y-1});
    if (itBack != chunkColumn.end()){
        chunkColumBack = itBack->second;
    }
    auto itLeft = chunkColumn.find({chunkColumnPos.x-1, chunkColumnPos.y});
    if (itLeft != chunkColumn.end()){
        chunkColumLeft = itLeft->second;
    }
    auto itRight = chunkColumn.find({chunkColumnPos.x+1, chunkColumnPos.y});
    if (itRight != chunkColumn.end()){
        chunkColumRight = itRight->second;
    }
    
    {
        std::lock_guard<std::mutex> lock(chunksToGenerateMeshQueue_mutex);
        chunksToGenerateMeshQueue.push_front({it->second, position.y, chunkColumFront, chunkColumBack, chunkColumLeft, chunkColumRight, timeStamp++}); // priority meshing, timestamp 0
    }
    chunksToGenerateMeshQueue_cv.notify_one();

}

void World::loadTerrain() {
    
    while (!receivedChunks.empty()) {

        auto chunkPacket = receivedChunks.front();
        receivedChunks.pop();

        if (chunkPacket.y < 0 || chunkPacket.y >= WorldConfig::CHUNK_COLUMN_HEIGHT) {
            continue;
        }

        glm::ivec2 chunkColumnPos = {chunkPacket.x, chunkPacket.z};
        auto it = chunkColumn.find(chunkColumnPos);
        
        //std::cout << "Received chunk at position: (" << chunkPacket.x << ", " << chunkPacket.y << ", " << chunkPacket.z << ")" << std::endl;

        if (it == chunkColumn.end()){
            continue; // chunk column not loaded, ignore
        }
        
        //std::cout << "Loading chunk at position: (" << chunkPacket.x << ", " << chunkPacket.y << ", " << chunkPacket.z << ")" << std::endl;

        for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++) {
            for (int y = 0; y < WorldConfig::CHUNK_SIZE; y++) {
                for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++) {
                    it->second->blocks[x][y + chunkPacket.y * WorldConfig::CHUNK_SIZE][z] = chunkPacket.chunkBlocks[x * WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_SIZE + y * WorldConfig::CHUNK_SIZE + z];
                }
            }
        }
        it->second->loadedChunks++;
        if (it->second->loadedChunks == WorldConfig::CHUNK_COLUMN_HEIGHT){
            chunksToGenerateMesh.push(it->first);
#ifdef DEBUG
            it->second->debugState = ChunkColumn::ChunkDebugState::InMeshingQueue;
#endif
        }

    }


}

void World::generateMesh() {

    int chunksPushed = 0;

    
    while(!chunksGeneratedOnEdge.empty()){
        glm::ivec2 pos = *chunksGeneratedOnEdge.begin();
        chunksGeneratedOnEdge.erase(chunksGeneratedOnEdge.begin());
        auto column = chunkColumn.find(pos);
        if (column == chunkColumn.end()){
            continue; // chunk column not loaded, ignore
        }
        std::shared_ptr<ChunkColumn> 
        chunkColumFront(nullptr), 
        chunkColumBack(nullptr), 
        chunkColumLeft(nullptr), 
        chunkColumRight(nullptr);
        auto it = chunkColumn.find({pos.x-1, pos.y});
        if (it != chunkColumn.end()){
            chunkColumLeft = it->second;
        }
        it = chunkColumn.find({pos.x+1, pos.y});
        if (it != chunkColumn.end()){
            chunkColumRight = it->second;
        }
        it = chunkColumn.find({pos.x, pos.y+1});
        if (it != chunkColumn.end()){
            chunkColumFront = it->second;
        }
        it = chunkColumn.find({pos.x, pos.y-1});
        if (it != chunkColumn.end()){
            chunkColumBack = it->second;
        }
        
        {
            std::lock_guard<std::mutex> lock(chunksToGenerateMeshQueue_mutex);
            for (int height = 0; height < WorldConfig::CHUNK_COLUMN_HEIGHT; height++){
                chunksToGenerateMeshQueue.push_back({column->second, height, chunkColumFront, chunkColumBack, chunkColumLeft, chunkColumRight, timeStamp++});
                chunksPushed++;
            }
        }
        
        
    }
    
    while(!chunksToGenerateMesh.empty()){

        glm::ivec2 pos = chunksToGenerateMesh.top();
        chunksToGenerateMesh.pop();
        auto column = chunkColumn.find(pos);
        if (column == chunkColumn.end()){
            continue; // chunk column not loaded, ignore
        }
        std::shared_ptr<ChunkColumn> 
            chunkColumFront(nullptr), 
            chunkColumBack(nullptr), 
            chunkColumLeft(nullptr), 
            chunkColumRight(nullptr);
        auto it = chunkColumn.find({pos.x-1, pos.y});
        if (it != chunkColumn.end()){
            chunkColumLeft = it->second;
            if (chunksGeneratedOnEdge.find({pos.x-1, pos.y}) == chunksGeneratedOnEdge.end()){
                chunksGeneratedOnEdge.insert({pos.x-1, pos.y});
            }
        }
        it = chunkColumn.find({pos.x+1, pos.y});
        if (it != chunkColumn.end()){
            chunkColumRight = it->second;
            if (chunksGeneratedOnEdge.find({pos.x+1, pos.y}) == chunksGeneratedOnEdge.end()){
                chunksGeneratedOnEdge.insert({pos.x+1, pos.y});
            }
        }
        it = chunkColumn.find({pos.x, pos.y+1});
        if (it != chunkColumn.end()){
            chunkColumFront = it->second;
            if (chunksGeneratedOnEdge.find({pos.x, pos.y+1}) == chunksGeneratedOnEdge.end()){
                chunksGeneratedOnEdge.insert({pos.x, pos.y+1});
            }
        }
        it = chunkColumn.find({pos.x, pos.y-1});
        if (it != chunkColumn.end()){
            chunkColumBack = it->second;
            if (chunksGeneratedOnEdge.find({pos.x, pos.y-1}) == chunksGeneratedOnEdge.end()){
                chunksGeneratedOnEdge.insert({pos.x, pos.y-1});
            }
        }

        {
            std::lock_guard<std::mutex> lock(chunksToGenerateMeshQueue_mutex);
            for (int height = 0; height < WorldConfig::CHUNK_COLUMN_HEIGHT; height++){
                chunksToGenerateMeshQueue.push_back({column->second, height, chunkColumFront, chunkColumBack, chunkColumLeft, chunkColumRight, timeStamp++});
            }
        }
        
    }


    if (chunksPushed > 0){
        chunksToGenerateMeshQueue_cv.notify_one();
    }

}

void World::loadChunks(glm::vec3 playerPosition) {
    
    //std::cout << "Loading chunks around player position: " << playerPosition.x << ", " << playerPosition.y << ", " << playerPosition.z << std::endl;
    glm::ivec2 chunkPosition(
        static_cast<int>(std::floor(playerPosition.x / WorldConfig::CHUNK_SIZE)),
        static_cast<int>(std::floor(playerPosition.z / WorldConfig::CHUNK_SIZE))
    );

    if (chunkPosition == lastPlayerChunkPosition && chunkColumn.find(chunkPosition) != chunkColumn.end()) {
        return; // player hasn't moved to a new chunk column, no need to load new chunks
    }
    lastPlayerChunkPosition = chunkPosition;

    int pt2 = 0;
    for (auto it = chunkColumn.begin(); it != chunkColumn.end();) {
        auto pos = it->first;
        int x = pos.x;
        int z = pos.y;
        if ((x - chunkPosition.x) * (x - chunkPosition.x) + (z - chunkPosition.y) * (z - chunkPosition.y) > renderDistance * renderDistance) {
            for (int j = 0; j < WorldConfig::CHUNK_COLUMN_HEIGHT; j++) {
                freeChunkPositionIndices.push(it->second->chunks[j].ssboIndex);
            }
            it = chunkColumn.erase(it);
        } else {
            ++it;
        }
    }

    for (auto offset : chunksToLoadOffset){        
        glm::ivec2 chunkColumnPos = offset + chunkPosition;
        auto it = chunkColumn.find(chunkColumnPos);
        if (it == chunkColumn.end()) {

            chunkColumn[chunkColumnPos] = std::make_shared<ChunkColumn>(chunkColumnPos, &vramAllocator);
            generateTerrain(*chunkColumn[chunkColumnPos]);

        }
    }
    
}

void World::meshGenerationWorkerThread() {

    while (true) {

        // Take job from queue
        MeshJob job[meshesToGeneratePerThread];
        int jobCount = 0;
        {
            std::unique_lock<std::mutex> lock(chunksToGenerateMeshQueue_mutex);
            chunksToGenerateMeshQueue_cv.wait(lock, [this] { return (!chunksToGenerateMeshQueue.empty() || !meshGenerationThreadsRunning); });
            if (!meshGenerationThreadsRunning && chunksToGenerateMeshQueue.empty()) {
                return; // Exit the thread if the flag is false and there are no more jobs
            }
            while (!chunksToGenerateMeshQueue.empty() && jobCount < meshesToGeneratePerThread) {
                job[jobCount] = chunksToGenerateMeshQueue.front();
                chunksToGenerateMeshQueue.pop_front();
                jobCount++;
            }
        }

        // Generate meshes for the jobs
        for (int i = 0; i < jobCount; i++) {
            {
                std::lock_guard<std::mutex> lock(job[i].chunkColumn->chunks[job[i].columnHeight].meshMutex);
                if (job[i].chunkColumn->chunks[job[i].columnHeight].timeStamp > job[i].timestamp) {
                    continue; // Skip this job if a newer job has already been processed for this chunk
                }
                job[i].chunkColumn->chunks[job[i].columnHeight].timeStamp = job[i].timestamp;
                generateMeshForChunkThreadSafe(job[i].chunkColumn, job[i].columnHeight, job[i].chunkColumn_Front, job[i].chunkColumn_Back, job[i].chunkColumn_Left, job[i].chunkColumn_Right);
                job[i].chunkColumn->chunks[job[i].columnHeight].hasMeshToUpload = true;
            }
        }

        // Send back meshed to be uploaded to GPU
        {
            std::lock_guard<std::mutex> lock(chunksToUploadToGPU_mutex);
            for (int i = 0; i < jobCount; i++) {
                glm::ivec3 chunkPos(job[i].chunkColumn->position.x, job[i].columnHeight, job[i].chunkColumn->position.y);
                chunksToUploadToGPU.push(chunkPos);
            }
        }

    }

}

void World::uploadChunksToGPU() {

    int chunksUploadedThisFrame = 0;
    while (chunksUploadedThisFrame < chunksToUploadToGPUPerFrame) {
        glm::ivec3 chunkPos;
        {
            std::lock_guard<std::mutex> lock(chunksToUploadToGPU_mutex);
            if (chunksToUploadToGPU.empty()) break;
            chunkPos = chunksToUploadToGPU.front();
            chunksToUploadToGPU.pop();
        }
        auto it = chunkColumn.find({chunkPos.x, chunkPos.z});
        if (it == chunkColumn.end()) {
            continue; // chunk column not loaded, ignore
        }
        if (chunkPos.y < 0 || chunkPos.y >= WorldConfig::CHUNK_COLUMN_HEIGHT) {
            continue; // invalid chunk height, ignore
        }
        auto &chunk = it->second->chunks[chunkPos.y];
        if (chunk.ssboIndex == -1) {
            continue; // chunk not assigned an SSBO index, ignore
        }
        
        std::unique_lock<std::mutex> lock(chunk.meshMutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            continue; // Worker is currently meshing this chunk, ignore and try again later
        }

        if (!chunk.hasMeshToUpload) {
            continue; // no mesh to upload, ignore
        }

        chunk.indiciesCountOpaque = chunk.indiciesOpaque.size();
        chunk.verticesCountOpaque = chunk.verticesOpaque.size();

        if (chunk.verticesCountOpaque > 0) {
            if (chunk.chunkAllocationOpaque.vertexCount < chunk.verticesOpaque.size() || chunk.chunkAllocationOpaque.indexCount < chunk.indiciesOpaque.size()){
                if (chunk.chunkAllocationOpaque.vertexBlocks > 0 && chunk.chunkAllocationOpaque.indexBlocks > 0){
                    vramAllocator.free(chunk.chunkAllocationOpaque);
                }
                chunk.chunkAllocationOpaque = vramAllocator.allocate(chunk.verticesCountOpaque, chunk.indiciesCountOpaque);
            }
            glNamedBufferSubData(vramAllocator.getVertexBufferID(), chunk.chunkAllocationOpaque.vertexBlocks * sizeof(uint32_t), chunk.verticesCountOpaque * sizeof(uint32_t), chunk.verticesOpaque.data());
            glNamedBufferSubData(vramAllocator.getIndexBufferID(), chunk.chunkAllocationOpaque.indexBlocks * sizeof(uint16_t), chunk.indiciesCountOpaque * sizeof(uint16_t), chunk.indiciesOpaque.data());
        }

        chunk.indiciesCountTransparent = chunk.indiciesTransparent.size();
        chunk.verticesCountTransparent = chunk.verticesTransparent.size();

        if (chunk.verticesCountTransparent > 0) {
            if (chunk.chunkAllocationTransparent.vertexCount < chunk.verticesTransparent.size() || chunk.chunkAllocationTransparent.indexCount < chunk.indiciesTransparent.size()){
                if (chunk.chunkAllocationTransparent.vertexBlocks > 0 && chunk.chunkAllocationTransparent.indexBlocks > 0){
                    vramAllocator.free(chunk.chunkAllocationTransparent);
                }
                chunk.chunkAllocationTransparent = vramAllocator.allocate(chunk.verticesCountTransparent, chunk.indiciesCountTransparent);
            }
            glNamedBufferSubData(vramAllocator.getVertexBufferID(), chunk.chunkAllocationTransparent.vertexBlocks * sizeof(uint32_t), chunk.verticesCountTransparent * sizeof(uint32_t), chunk.verticesTransparent.data());
            glNamedBufferSubData(vramAllocator.getIndexBufferID(), chunk.chunkAllocationTransparent.indexBlocks * sizeof(uint16_t), chunk.indiciesCountTransparent * sizeof(uint16_t), chunk.indiciesTransparent.data());
        }

        //std::cout << "Uploaded chunk at position: (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ") to GPU. Opaque vertices: " << chunk.verticesCountOpaque << ", Opaque indices: " << chunk.indiciesCountOpaque << ", Transparent vertices: " << chunk.verticesCountTransparent << ", Transparent indices: " << chunk.indiciesCountTransparent << std::endl;

        chunk.verticesOpaque.clear();
        chunk.indiciesOpaque.clear();
        chunk.verticesTransparent.clear();
        chunk.indiciesTransparent.clear();

        chunk.isMeshGenerated = true;
        chunk.hasMeshToUpload = false; // Reset the flag after uploading to GPU

#ifdef DEBUG
        bool allMeshed = true;
        for (int i = 0; i < WorldConfig::CHUNK_COLUMN_HEIGHT; i++) {
            if (!it->second->chunks[i].isMeshGenerated) {
                allMeshed = false;
                break;
            }
        }
        if (allMeshed) {
            it->second->debugState = ChunkColumn::ChunkDebugState::Ready;
        }
#endif

        chunksUploadedThisFrame++;
    }
    
}

void World::render(Shader& shader, glm::vec3 playerPosition) {

    static bool firstRender = true;

    std::vector<DrawElementsIndirectCommand> drawCommandsOpaque;
    for (auto offset : chunksToLoadOffset){
        glm::ivec2 chunkColumnPos  = {static_cast<int>(std::floor(playerPosition.x / WorldConfig::CHUNK_SIZE)) + offset.x, static_cast<int>(std::floor(playerPosition.z / WorldConfig::CHUNK_SIZE)) + offset.y};
        auto it = chunkColumn.find(chunkColumnPos);
        if (it == chunkColumn.end()) continue; // chunk column not loaded, ignore
        auto& chunkColumn = it->second;
        for (int i = 0; i < WorldConfig::CHUNK_COLUMN_HEIGHT; i++){
            if (chunkColumn->chunks[i].indiciesCountOpaque == 0 || chunkColumn->chunks[i].ssboIndex == -1 || !chunkColumn->chunks[i].isMeshGenerated) continue;
            DrawElementsIndirectCommand cmd;
            cmd.count = chunkColumn->chunks[i].indiciesCountOpaque;
            cmd.instanceCount = 1;
            cmd.firstIndex = chunkColumn->chunks[i].chunkAllocationOpaque.indexBlocks;
            cmd.baseVertex = chunkColumn->chunks[i].chunkAllocationOpaque.vertexBlocks;
            cmd.baseInstance = chunkColumn->chunks[i].ssboIndex;
            drawCommandsOpaque.push_back(cmd);
        }
    }

    std::vector<DrawElementsIndirectCommand> drawCommandsTransparent;
    for (auto offset = chunksToLoadOffset.rbegin(); offset != chunksToLoadOffset.rend(); ++offset){
        glm::ivec2 chunkColumnPos  = {static_cast<int>(std::floor(playerPosition.x / WorldConfig::CHUNK_SIZE)) + offset->x, static_cast<int>(std::floor(playerPosition.z / WorldConfig::CHUNK_SIZE)) + offset->y};
        auto it = chunkColumn.find(chunkColumnPos);
        if (it == chunkColumn.end()) continue; // chunk column not loaded, ignore
        auto& chunkColumn = it->second;
        for (int j = 0; j < WorldConfig::CHUNK_COLUMN_HEIGHT; j++){
            if (chunkColumn->chunks[j].indiciesCountTransparent == 0 || chunkColumn->chunks[j].ssboIndex == -1 || !chunkColumn->chunks[j].isMeshGenerated) continue;
            DrawElementsIndirectCommand cmd;
            cmd.count = chunkColumn->chunks[j].indiciesCountTransparent;
            cmd.instanceCount = 1;
            cmd.firstIndex = chunkColumn->chunks[j].chunkAllocationTransparent.indexBlocks;
            cmd.baseVertex = chunkColumn->chunks[j].chunkAllocationTransparent.vertexBlocks;
            cmd.baseInstance = chunkColumn->chunks[j].ssboIndex;
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
    auto it = chunkColumn.find({chunkColumnX, chunkColumnZ});
    if (it == chunkColumn.end()) {
        return Block::AIR; // Chunk column not loaded
    }
    if (position.y < 0 || position.y >= WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT) {
        return Block::AIR; // Y position out of bounds
    }
    int chunkPosX = position.x - chunkColumnX * WorldConfig::CHUNK_SIZE;
    int chunkPosZ = position.z - chunkColumnZ * WorldConfig::CHUNK_SIZE;
    return it->second->blocks[chunkPosX][position.y][chunkPosZ];

}

void World::clean(){
    chunkColumn.clear();
    textureArray.clean();
}

World::~World() {
    chunkColumn.clear();
}

#ifdef DEBUG
void World::renderDebugWindow(glm::vec3 playerPosition) {
    ImGui::Begin("Chunk Status");
    
    glm::ivec2 playerChunkPos(
        static_cast<int>(std::floor(playerPosition.x / WorldConfig::CHUNK_SIZE)),
        static_cast<int>(std::floor(playerPosition.z / WorldConfig::CHUNK_SIZE))
    );

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    
    float quadSize = 10.0f;
    float spacing = 2.0f;
    
    for (int x = -renderDistance; x <= renderDistance; x++) {
        for (int z = -renderDistance; z <= renderDistance; z++) {
            glm::ivec2 chunkPos = playerChunkPos + glm::ivec2(x, z);
            
            ImU32 color = IM_COL32(50, 50, 50, 255);
            
            if (x*x + z*z <= renderDistance * renderDistance) {
                auto it = chunkColumn.find(chunkPos);
                if (it == chunkColumn.end()) {
                    color = IM_COL32(100, 100, 100, 255);
                } else {
                    auto& col = it->second;
                    switch (col->debugState.load()) {
                        case ChunkColumn::ChunkDebugState::Requested:
                            color = IM_COL32(200, 200, 50, 255);
                            break;
                        case ChunkColumn::ChunkDebugState::InMeshingQueue:
                            color = IM_COL32(200, 100, 50, 255);
                            break;
                        case ChunkColumn::ChunkDebugState::Ready:
                            color = IM_COL32(50, 200, 50, 255);
                            break;
                        default:
                            color = IM_COL32(100, 100, 100, 255);
                            break;
                    }
                }
            }
            
            float px = p.x + (x + renderDistance) * (quadSize + spacing);
            float py = p.y + (z + renderDistance) * (quadSize + spacing);
            draw_list->AddRectFilled(ImVec2(px, py), ImVec2(px + quadSize, py + quadSize), color);
            
            if (x == 0 && z == 0) {
                draw_list->AddRect(ImVec2(px, py), ImVec2(px + quadSize, py + quadSize), IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);
            }
        }
    }
    
    float totalSize = (renderDistance * 2 + 1) * (quadSize + spacing);
    ImGui::Dummy(ImVec2(totalSize, totalSize));
    
    ImGui::TextColored(ImVec4(100/255.f, 100/255.f, 100/255.f, 1.f), "Unrequested / Wait");
    ImGui::TextColored(ImVec4(200/255.f, 200/255.f, 50/255.f, 1.f), "Requested");
    ImGui::TextColored(ImVec4(200/255.f, 100/255.f, 50/255.f, 1.f), "In Meshing Queue");
    ImGui::TextColored(ImVec4(50/255.f, 200/255.f, 50/255.f, 1.f), "Ready");
    
    ImGui::End();
}
#endif
