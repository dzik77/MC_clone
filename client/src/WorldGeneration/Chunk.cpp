#include <WorldGeneration/Chunk.hpp>
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
        Block (*blocks)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE], int columnHeight,
        Block (*blocks_Front)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE],
        Block (*blocks_Back)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE],
        Block (*blocks_Left)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE],
        Block (*blocks_Right)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE])       {
    
    std::vector<uint32_t> vertices;
    std::vector<uint16_t> indicies;
    
    int yOffset = columnHeight * WorldConfig::CHUNK_SIZE;
    
    for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++){
        for (int y = 0; y < WorldConfig::CHUNK_SIZE; y++){
            for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++){
                if (blocks[x][y + yOffset][z] == Block::AIR) continue;
                if ((x == 0 ? (blocks_Left == nullptr || blocks_Left[WorldConfig::CHUNK_SIZE-1][y + yOffset][z] == Block::AIR) :  blocks[x-1][y + yOffset][z] == Block::AIR)){ // LEFT
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y+1,z,getTextureID(blocks[x][y + yOffset][z], Direction::LEFT), (int)Direction::LEFT, 0));   // TL
                    vertices.emplace_back(create_vertex(x,y+1,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::LEFT), (int)Direction::LEFT, 1)); // TR
                    vertices.emplace_back(create_vertex(x,y,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::LEFT), (int)Direction::LEFT, 2));   // BR
                    vertices.emplace_back(create_vertex(x,y,z,getTextureID(blocks[x][y + yOffset][z], Direction::LEFT), (int)Direction::LEFT, 3));     // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if ((x == WorldConfig::CHUNK_SIZE-1 ? (blocks_Right == nullptr || blocks_Right[0][y + yOffset][z] == Block::AIR) : blocks[x+1][y + yOffset][z] == Block::AIR)){ // RIGHT
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x+1,y+1,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::RIGHT), (int)Direction::RIGHT, 0)); // TL
                    vertices.emplace_back(create_vertex(x+1,y+1,z,getTextureID(blocks[x][y + yOffset][z], Direction::RIGHT), (int)Direction::RIGHT, 1));   // TR
                    vertices.emplace_back(create_vertex(x+1,y,z,getTextureID(blocks[x][y + yOffset][z], Direction::RIGHT), (int)Direction::RIGHT, 2));     // BR
                    vertices.emplace_back(create_vertex(x+1,y,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::RIGHT), (int)Direction::RIGHT, 3));   // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if ((y == 0 ? (columnHeight == 0 || blocks[x][y-1 + yOffset][z] == Block::AIR) : blocks[x][y-1 + yOffset][z] == Block::AIR)){ // DOWN
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::DOWN), (int)Direction::DOWN, 0));   // TL
                    vertices.emplace_back(create_vertex(x+1,y,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::DOWN), (int)Direction::DOWN, 1)); // TR
                    vertices.emplace_back(create_vertex(x+1,y,z,getTextureID(blocks[x][y + yOffset][z], Direction::DOWN), (int)Direction::DOWN, 2));   // BR
                    vertices.emplace_back(create_vertex(x,y,z,getTextureID(blocks[x][y + yOffset][z], Direction::DOWN), (int)Direction::DOWN, 3));     // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if ((y == WorldConfig::CHUNK_SIZE-1 ? (columnHeight == WorldConfig::CHUNK_COLUMN_HEIGHT-1 || blocks[x][y+1 + yOffset][z] == Block::AIR) : blocks[x][y+1 + yOffset][z] == Block::AIR)){ // UP
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y+1,z,getTextureID(blocks[x][y + yOffset][z], Direction::UP), (int)Direction::UP, 0));     // TL
                    vertices.emplace_back(create_vertex(x+1,y+1,z,getTextureID(blocks[x][y + yOffset][z], Direction::UP), (int)Direction::UP, 1));   // TR
                    vertices.emplace_back(create_vertex(x+1,y+1,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::UP), (int)Direction::UP, 2)); // BR
                    vertices.emplace_back(create_vertex(x,y+1,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::UP), (int)Direction::UP, 3));   // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if ((z == 0 ? (blocks_Back == nullptr || blocks_Back[x][y + yOffset][WorldConfig::CHUNK_SIZE-1] == Block::AIR) : blocks[x][y + yOffset][z-1] == Block::AIR)){ // BACKWARD
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x+1,y+1,z,getTextureID(blocks[x][y + yOffset][z], Direction::BACKWARD), (int)Direction::BACKWARD, 0)); // TL
                    vertices.emplace_back(create_vertex(x,y+1,z,getTextureID(blocks[x][y + yOffset][z], Direction::BACKWARD), (int)Direction::BACKWARD, 1));   // TR
                    vertices.emplace_back(create_vertex(x,y,z,getTextureID(blocks[x][y + yOffset][z], Direction::BACKWARD), (int)Direction::BACKWARD, 2));     // BR
                    vertices.emplace_back(create_vertex(x+1,y,z,getTextureID(blocks[x][y + yOffset][z], Direction::BACKWARD), (int)Direction::BACKWARD, 3));   // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if ((z == WorldConfig::CHUNK_SIZE-1 ? (blocks_Front == nullptr || blocks_Front[x][y + yOffset][0] == Block::AIR) : blocks[x][y + yOffset][z+1] == Block::AIR)){ // FORWARD
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y+1,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::FORWARD), (int)Direction::FORWARD, 0)); // TL
                    vertices.emplace_back(create_vertex(x+1,y+1,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::FORWARD), (int)Direction::FORWARD, 1)); // TR
                    vertices.emplace_back(create_vertex(x+1,y,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::FORWARD), (int)Direction::FORWARD, 2));   // BR
                    vertices.emplace_back(create_vertex(x,y,z+1,getTextureID(blocks[x][y + yOffset][z], Direction::FORWARD), (int)Direction::FORWARD, 3));     // BL
                    
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
    std::memset(blocks, 0, sizeof(blocks));
    for (int i = 0; i < WorldConfig::CHUNK_COLUMN_HEIGHT; i++) {
        chunks[i].init(vramAllocator);
    }
}

ChunkColumn::~ChunkColumn()
{
}

ChunkColumn::ChunkColumn(ChunkColumn &&other) noexcept 
    : position(other.position) 
{
    std::memcpy(blocks, other.blocks, sizeof(blocks));
    for (int i = 0; i < WorldConfig::CHUNK_COLUMN_HEIGHT; i++) {
        chunks[i] = std::move(other.chunks[i]);
    }
}

ChunkColumn &ChunkColumn::operator=(ChunkColumn &&other) noexcept
{
    if (this != &other) {
        position = other.position;
        std::memcpy(blocks, other.blocks, sizeof(blocks));
        for (int i = 0; i < WorldConfig::CHUNK_COLUMN_HEIGHT; i++) {
            chunks[i] = std::move(other.chunks[i]);
        }
    }
    return *this;
}