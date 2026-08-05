#include "Chunk.hpp"
#include <glad/glad.h>
#include <cstring>

Chunk::Chunk() {
    vramAllocator = nullptr;
    chunkAllocation = {0, 0, 0, 0};
}
Chunk::Chunk(VramAllocator* vramAllocator) : vramAllocator(vramAllocator) {
    chunkAllocation = {0, 0, 0, 0};
}
Chunk::Chunk(Chunk&& other) noexcept 
    : verticesCount(other.verticesCount), indiciesCount(other.indiciesCount), ssboIndex(other.ssboIndex), vramAllocator(other.vramAllocator), chunkAllocation(other.chunkAllocation) {
    other.vramAllocator = nullptr;
    other.chunkAllocation = {0, 0, 0, 0};
    other.verticesCount = 0;
    other.indiciesCount = 0;
}

Chunk& Chunk::operator=(Chunk&& other) noexcept {
    if (this != &other) {
        if (vramAllocator && (chunkAllocation.vertexCount > 0 || chunkAllocation.indexCount > 0)) {
            vramAllocator->free(chunkAllocation);
        }
        verticesCount = other.verticesCount;
        indiciesCount = other.indiciesCount;
        ssboIndex = other.ssboIndex;
        chunkAllocation = other.chunkAllocation;
        vramAllocator = other.vramAllocator;

        other.vramAllocator = nullptr;
        other.chunkAllocation = {0, 0, 0, 0};
        other.verticesCount = 0;
        other.indiciesCount = 0;
    }
    return *this;
}

Chunk::~Chunk(){
    if (vramAllocator && (chunkAllocation.vertexCount > 0 || chunkAllocation.indexCount > 0)) {
        vramAllocator->free(chunkAllocation);
    }
}

void Chunk::init(VramAllocator *vramAllocator){
    this->vramAllocator = vramAllocator;
}

void Chunk::generateMesh(
        uint8_t (*Blocks)[CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE], int columnHeight,
        uint8_t (*Blocks_Front)[CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE],
        uint8_t (*Blocks_Back)[CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE],
        uint8_t (*Blocks_Left)[CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE],
        uint8_t (*Blocks_Right)[CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE])       {
    
    std::vector<uint32_t> vertices;
    std::vector<uint16_t> indicies;
    
    int yOffset = columnHeight * CHUNK_SIZE;
    
    for (int x = 0; x < CHUNK_SIZE; x++){
        for (int y = 0; y < CHUNK_SIZE; y++){
            for (int z = 0; z < CHUNK_SIZE; z++){
                if (Blocks[x][y + yOffset][z] == 0) continue;
                if ((x == 0 ? (Blocks_Left == nullptr || Blocks_Left[CHUNK_SIZE-1][y + yOffset][z] == 0) :  Blocks[x-1][y + yOffset][z] == 0)){ // LEFT
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::LEFT, 0));   // TL
                    vertices.emplace_back(create_vertex(x,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::LEFT, 1)); // TR
                    vertices.emplace_back(create_vertex(x,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::LEFT, 2));   // BR
                    vertices.emplace_back(create_vertex(x,y,z,Blocks[x][y + yOffset][z], (int)Direction::LEFT, 3));     // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if ((x == CHUNK_SIZE-1 ? (Blocks_Right == nullptr || Blocks_Right[0][y + yOffset][z] == 0) : Blocks[x+1][y + yOffset][z] == 0)){ // RIGHT
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x+1,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::RIGHT, 0)); // TL
                    vertices.emplace_back(create_vertex(x+1,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::RIGHT, 1));   // TR
                    vertices.emplace_back(create_vertex(x+1,y,z,Blocks[x][y + yOffset][z], (int)Direction::RIGHT, 2));     // BR
                    vertices.emplace_back(create_vertex(x+1,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::RIGHT, 3));   // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if ((y == 0 ? (columnHeight == 0 || Blocks[x][y-1 + yOffset][z] == 0) : Blocks[x][y-1 + yOffset][z] == 0)){ // DOWN
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::DOWN, 0));   // TL
                    vertices.emplace_back(create_vertex(x+1,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::DOWN, 1)); // TR
                    vertices.emplace_back(create_vertex(x+1,y,z,Blocks[x][y + yOffset][z], (int)Direction::DOWN, 2));   // BR
                    vertices.emplace_back(create_vertex(x,y,z,Blocks[x][y + yOffset][z], (int)Direction::DOWN, 3));     // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if ((y == CHUNK_SIZE-1 ? (columnHeight == CHUNK_COLUMN_HEIGHT-1 || Blocks[x][y+1 + yOffset][z] == 0) : Blocks[x][y+1 + yOffset][z] == 0)){ // UP
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::UP, 0));     // TL
                    vertices.emplace_back(create_vertex(x+1,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::UP, 1));   // TR
                    vertices.emplace_back(create_vertex(x+1,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::UP, 2)); // BR
                    vertices.emplace_back(create_vertex(x,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::UP, 3));   // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if ((z == 0 ? (Blocks_Back == nullptr || Blocks_Back[x][y + yOffset][CHUNK_SIZE-1] == 0) : Blocks[x][y + yOffset][z-1] == 0)){ // BACKWARD
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x+1,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::BACKWARD, 0)); // TL
                    vertices.emplace_back(create_vertex(x,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::BACKWARD, 1));   // TR
                    vertices.emplace_back(create_vertex(x,y,z,Blocks[x][y + yOffset][z], (int)Direction::BACKWARD, 2));     // BR
                    vertices.emplace_back(create_vertex(x+1,y,z,Blocks[x][y + yOffset][z], (int)Direction::BACKWARD, 3));   // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if ((z == CHUNK_SIZE-1 ? (Blocks_Front == nullptr || Blocks_Front[x][y + yOffset][0] == 0) : Blocks[x][y + yOffset][z+1] == 0)){ // FORWARD
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::FORWARD, 0)); // TL
                    vertices.emplace_back(create_vertex(x+1,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::FORWARD, 1)); // TR
                    vertices.emplace_back(create_vertex(x+1,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::FORWARD, 2));   // BR
                    vertices.emplace_back(create_vertex(x,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::FORWARD, 3));     // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
            }
        }
    }
    indiciesCount = indicies.size();
    verticesCount = vertices.size();

    if (chunkAllocation.vertexCount < vertices.size() || chunkAllocation.vertexCount < indicies.size()){
        if (chunkAllocation.vertexBlocks > 0 && chunkAllocation.indexBlocks > 0){
            vramAllocator->free(chunkAllocation);
        }
        chunkAllocation = vramAllocator->allocate(verticesCount, indiciesCount);
    }
    //std::memcpy(vramAllocator->getVertexData() + chunkAllocation.vertexBlocks, vertices.data(), verticesCount * sizeof(uint32_t));
    //std::memcpy(vramAllocator->getIndexData() + chunkAllocation.indexBlocks, indicies.data(), indiciesCount * sizeof(uint16_t));
    glNamedBufferSubData(vramAllocator->getVertexBufferID(), chunkAllocation.vertexBlocks * sizeof(uint32_t), verticesCount * sizeof(uint32_t), vertices.data());
    glNamedBufferSubData(vramAllocator->getIndexBufferID(), chunkAllocation.indexBlocks * sizeof(uint16_t), indiciesCount * sizeof(uint16_t), indicies.data());

}

ChunkColumn::ChunkColumn(glm::vec2 position, VramAllocator* vramAllocator) 
    : position(position), vramAllocator(vramAllocator)
{
    std::memset(Blocks, 0, sizeof(Blocks));
    for (int i = 0; i < CHUNK_COLUMN_HEIGHT; i++) {
        chunks[i].init(vramAllocator);
    }
}

ChunkColumn::~ChunkColumn()
{
}

ChunkColumn::ChunkColumn(ChunkColumn &&other) noexcept 
    : position(other.position) 
{
    std::memcpy(Blocks, other.Blocks, sizeof(Blocks));
    for (int i = 0; i < CHUNK_COLUMN_HEIGHT; i++) {
        chunks[i] = std::move(other.chunks[i]);
    }
}

ChunkColumn &ChunkColumn::operator=(ChunkColumn &&other) noexcept
{
    if (this != &other) {
        position = other.position;
        std::memcpy(Blocks, other.Blocks, sizeof(Blocks));
        for (int i = 0; i < CHUNK_COLUMN_HEIGHT; i++) {
            chunks[i] = std::move(other.chunks[i]);
        }
    }
    return *this;
}