#include <WorldGeneration/World.hpp>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <stdexcept>

const std::vector<std::filesystem::path> texturePaths = {
    "textures/stone.png",
    "textures/dirt.png",
    "textures/grass_block_side.png",
    "textures/grass_block_top_item.png"
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
    
    indirectCommandBufferSize = PREALLOCATE_CHUNKS * sizeof(DrawElementsIndirectCommand) * 3;
    glCreateBuffers(1, &indirectCommandBuffer);
    glNamedBufferStorage(indirectCommandBuffer, indirectCommandBufferSize, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    mappedIndirectCommands = (DrawElementsIndirectCommand*)glMapNamedBufferRange(indirectCommandBuffer, 0, indirectCommandBufferSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    
    for (int i = 0; i < PREALLOCATE_CHUNKS; i++){
        freeChunkPositionIndices.push(i);
    }

}

void World::loadTextures() {
    textureArray.createTextureArray(texturePaths,16,16);
}

void World::generateTerrain(ChunkColumn &chunkColumn) {

    // load from server TODO:
    // i guess for now just auto wait for the chunk

    /*
    RequestChunkPacket requestPacket;
    requestPacket.chunkColumnX = chunkColumn.position.x;
    requestPacket.chunkColumnZ = chunkColumn.position.y;
    ENetPacket *packet = enet_packet_create(&requestPacket, sizeof(RequestChunkPacket), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_
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

void World::generateMesh() {

    std::stack<int> tempStack;
    while(!chunksGeneratedOnEdge.empty()){
        int idx = chunksGeneratedOnEdge.top();
        auto& chunkColumn = ChunkColumns[idx];
        chunksGeneratedOnEdge.pop();
        uint8_t (*Blocks_Front)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        uint8_t (*Blocks_Back)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        uint8_t (*Blocks_Left)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        uint8_t (*Blocks_Right)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        auto it = loadedChunkPositions.find({chunkColumn.position.x-1, chunkColumn.position.y});
        if (it != loadedChunkPositions.end()){
            Blocks_Left = ChunkColumns[it->second].Blocks;
        }
        it = loadedChunkPositions.find({chunkColumn.position.x+1, chunkColumn.position.y});
        if (it != loadedChunkPositions.end()){
            Blocks_Right = ChunkColumns[it->second].Blocks;
        }
        it = loadedChunkPositions.find({chunkColumn.position.x, chunkColumn.position.y+1});
        if (it != loadedChunkPositions.end()){
            Blocks_Front = ChunkColumns[it->second].Blocks;
        }
        it = loadedChunkPositions.find({chunkColumn.position.x, chunkColumn.position.y-1});
        if (it != loadedChunkPositions.end()){
            Blocks_Back = ChunkColumns[it->second].Blocks;
        }
        if (Blocks_Front == nullptr || Blocks_Back == nullptr || Blocks_Left == nullptr || Blocks_Right == nullptr){
            tempStack.push(idx);
            continue;
        }
        for (int height = 0; height < WorldConfig::CHUNK_COLUMN_HEIGHT; height++){
            chunkColumn.chunks[height].generateMesh(
                chunkColumn.Blocks, height,
                Blocks_Front,
                Blocks_Back,
                Blocks_Left,
                Blocks_Right
            );
        }
    }
    while(!tempStack.empty()){
        chunksGeneratedOnEdge.push(tempStack.top());
        tempStack.pop();
    }
    while(!chunksToGenerateMesh.empty()){
        int idx = chunksToGenerateMesh.top();
        auto& chunkColumn = ChunkColumns[idx];
        chunksToGenerateMesh.pop();
        uint8_t (*Blocks_Front)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        uint8_t (*Blocks_Back)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        uint8_t (*Blocks_Left)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        uint8_t (*Blocks_Right)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE] = nullptr;
        auto it = loadedChunkPositions.find({chunkColumn.position.x-1, chunkColumn.position.y});
        if (it != loadedChunkPositions.end()){
            Blocks_Left = ChunkColumns[it->second].Blocks;
        }
        it = loadedChunkPositions.find({chunkColumn.position.x+1, chunkColumn.position.y});
        if (it != loadedChunkPositions.end()){
            Blocks_Right = ChunkColumns[it->second].Blocks;
        }
        it = loadedChunkPositions.find({chunkColumn.position.x, chunkColumn.position.y+1});
        if (it != loadedChunkPositions.end()){
            Blocks_Front = ChunkColumns[it->second].Blocks;
        }
        it = loadedChunkPositions.find({chunkColumn.position.x, chunkColumn.position.y-1});
        if (it != loadedChunkPositions.end()){
            Blocks_Back = ChunkColumns[it->second].Blocks;
        }
        if (Blocks_Front == nullptr || Blocks_Back == nullptr || Blocks_Left == nullptr || Blocks_Right == nullptr){
            chunksGeneratedOnEdge.push(idx);
        }
        for (int height = 0; height < WorldConfig::CHUNK_COLUMN_HEIGHT; height++){
            chunkColumn.chunks[height].generateMesh(
                chunkColumn.Blocks, height,
                Blocks_Front,
                Blocks_Back,
                Blocks_Left,
                Blocks_Right
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
    for (int x = -renderDistance; x <= renderDistance; x++){
        for (int z = -renderDistance; z <= renderDistance; z++){
            if(x*x + z*z > renderDistance*renderDistance) 
                continue;
            if (loadedChunkPositions.find(std::pair<int,int>(chunkPosition.x + x, chunkPosition.z + z)) == loadedChunkPositions.end()){
                if (toRemove.size() > 0){
                    int idx = toRemove.back();
                    toRemove.pop_back();
                    for (int j = 0; j < WorldConfig::CHUNK_COLUMN_HEIGHT; j++) {
                        freeChunkPositionIndices.push(ChunkColumns[idx].chunks[j].ssboIndex);
                    }
                    loadedChunkPositions.erase(std::pair<int,int>(ChunkColumns[idx].position.x, ChunkColumns[idx].position.y));
                    ChunkColumns[idx] = ChunkColumn(glm::vec2(chunkPosition.x + x, chunkPosition.z + z), &vramAllocator);
                    generateTerrain(ChunkColumns[idx]);
                    loadedChunkPositions[std::pair<int,int>(chunkPosition.x + x, chunkPosition.z + z)] = idx;
                    chunksToGenerateMesh.push(idx);
                } else {
                    ChunkColumns.emplace_back(glm::vec2(chunkPosition.x + x, chunkPosition.z + z), &vramAllocator);
                    generateTerrain(ChunkColumns.back());
                    loadedChunkPositions[std::pair<int,int>(chunkPosition.x + x, chunkPosition.z + z)] = ChunkColumns.size()-1;
                    chunksToGenerateMesh.push(ChunkColumns.size()-1);
                }
            }
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
        
        if (idx != ChunkColumns.size() - 1) {
            int lastIdx = ChunkColumns.size() - 1;
            ChunkColumns[idx] = std::move(ChunkColumns[lastIdx]);
            loadedChunkPositions[std::pair<int,int>(ChunkColumns[idx].position.x, ChunkColumns[idx].position.y)] = idx;
        }
        ChunkColumns.pop_back();
    }
    //std::cout << "Chunks loaded around player position: " << playerPosition.x << ", " << playerPosition.y << ", " << playerPosition.z << std::endl;
}

void World::render(Shader& shader) {

    static bool firstRender = true;

    std::vector<DrawElementsIndirectCommand> drawCommands;
    for (auto& chunkColumn : ChunkColumns){
        for (int i = 0; i < WorldConfig::CHUNK_COLUMN_HEIGHT; i++){
            if (chunkColumn.chunks[i].indiciesCount == 0 || chunkColumn.chunks[i].ssboIndex == -1) continue;
            DrawElementsIndirectCommand cmd;
            cmd.count = chunkColumn.chunks[i].indiciesCount;
            cmd.instanceCount = 1;
            cmd.firstIndex = chunkColumn.chunks[i].chunkAllocation.indexBlocks;
            cmd.baseVertex = chunkColumn.chunks[i].chunkAllocation.vertexBlocks;
            cmd.baseInstance = chunkColumn.chunks[i].ssboIndex;
            drawCommands.push_back(cmd);
        }
    }

    static int currentFrame = 0;
    int bufferIndex = currentFrame % 3;
    currentFrame++;

    DrawElementsIndirectCommand* cmdPtr = mappedIndirectCommands + bufferIndex * PREALLOCATE_CHUNKS;
    std::memcpy(cmdPtr, drawCommands.data(), drawCommands.size() * sizeof(DrawElementsIndirectCommand));

    shader.use();
    glBindTextureUnit(0, textureArray.texture);
    glBindVertexArray(globalVAO);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectCommandBuffer);


    /*
    for (const auto& cmd : drawCommands) {
        glDrawElementsInstancedBaseVertexBaseInstance(
            GL_TRIANGLES, 
            cmd.count, 
            GL_UNSIGNED_SHORT, 
            (void*)(cmd.firstIndex * sizeof(uint16_t)), // Byte offset in EBO
            cmd.instanceCount, 
            cmd.baseVertex, 
            cmd.baseInstance
        );
    }
    */
    glMultiDrawElementsIndirect(
        GL_TRIANGLES, 
        GL_UNSIGNED_SHORT, 
        (void *)(bufferIndex * PREALLOCATE_CHUNKS * sizeof(DrawElementsIndirectCommand)), 
        drawCommands.size(), 
        0
    );

}

void World::clean(){
    ChunkColumns.clear();
    textureArray.clean();
}

World::~World() {
    ChunkColumns.clear();
}
