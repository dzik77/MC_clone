#include <WorldGeneration/Chunk.hpp>
#include <glad/glad.h>
#include <cstring>

Chunk::Chunk() {
    vramAllocator = nullptr;
    chunkAllocationOpaque = {0, 0, 0, 0};
    chunkAllocationTransparent = {0, 0, 0, 0};
}
Chunk::Chunk(VramAllocator* vramAllocator) : vramAllocator(vramAllocator) {
    chunkAllocationOpaque = {0, 0, 0, 0};
    chunkAllocationTransparent = {0, 0, 0, 0};
}
Chunk::Chunk(Chunk&& other) noexcept 
    : verticesCountOpaque(other.verticesCountOpaque), indiciesCountOpaque(other.indiciesCountOpaque),
      verticesCountTransparent(other.verticesCountTransparent), indiciesCountTransparent(other.indiciesCountTransparent),
      ssboIndex(other.ssboIndex), vramAllocator(other.vramAllocator),
      chunkAllocationOpaque(other.chunkAllocationOpaque), chunkAllocationTransparent(other.chunkAllocationTransparent) {
    other.vramAllocator = nullptr;
    other.chunkAllocationOpaque = {0, 0, 0, 0};
    other.chunkAllocationTransparent = {0, 0, 0, 0};
    other.verticesCountOpaque = 0;
    other.indiciesCountOpaque = 0;
    other.verticesCountTransparent = 0;
    other.indiciesCountTransparent = 0;
}

Chunk& Chunk::operator=(Chunk&& other) noexcept {
    if (this != &other) {
        if (vramAllocator && (chunkAllocationOpaque.vertexCount > 0 || chunkAllocationOpaque.indexCount > 0)) {
            vramAllocator->free(chunkAllocationOpaque);
        }
        if (vramAllocator && (chunkAllocationTransparent.vertexCount > 0 || chunkAllocationTransparent.indexCount > 0)) {
            vramAllocator->free(chunkAllocationTransparent);
        }
        verticesCountOpaque = other.verticesCountOpaque;
        indiciesCountOpaque = other.indiciesCountOpaque;
        verticesCountTransparent = other.verticesCountTransparent;
        indiciesCountTransparent = other.indiciesCountTransparent;
        ssboIndex = other.ssboIndex;
        chunkAllocationOpaque = other.chunkAllocationOpaque;
        chunkAllocationTransparent = other.chunkAllocationTransparent;
        vramAllocator = other.vramAllocator;

        other.vramAllocator = nullptr;
        other.chunkAllocationOpaque = {0, 0, 0, 0};
        other.chunkAllocationTransparent = {0, 0, 0, 0};
        other.verticesCountOpaque = 0;
        other.indiciesCountOpaque = 0;
        other.verticesCountTransparent = 0;
        other.indiciesCountTransparent = 0;
    }
    return *this;
}

Chunk::~Chunk(){
    if (vramAllocator && (chunkAllocationOpaque.vertexCount > 0 || chunkAllocationOpaque.indexCount > 0)) {
        vramAllocator->free(chunkAllocationOpaque);
    }
    if (vramAllocator && (chunkAllocationTransparent.vertexCount > 0 || chunkAllocationTransparent.indexCount > 0)) {
        vramAllocator->free(chunkAllocationTransparent);
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
    
    std::vector<uint32_t> verticesOpaque, verticesTransparent;
    std::vector<uint16_t> indiciesOpaque, indiciesTransparent;
    
    int yOffset = columnHeight * WorldConfig::CHUNK_SIZE;

    auto getNeighborLeft = [&](int x, int y, int z) -> Block {
        if (x == 0) return blocks_Left ? blocks_Left[WorldConfig::CHUNK_SIZE - 1][y + yOffset][z] : Block::AIR;
        return blocks[x - 1][y + yOffset][z];
    };

    auto getNeighborRight = [&](int x, int y, int z) -> Block {
        if (x == WorldConfig::CHUNK_SIZE - 1) return blocks_Right ? blocks_Right[0][y + yOffset][z] : Block::AIR;
        return blocks[x + 1][y + yOffset][z];
    };

    auto getNeighborDown = [&](int x, int y, int z) -> Block {
        if (y == 0) return (columnHeight == 0) ? Block::AIR : blocks[x][y - 1 + yOffset][z];
        return blocks[x][y - 1 + yOffset][z];
    };

    auto getNeighborUp = [&](int x, int y, int z) -> Block {
        if (y == WorldConfig::CHUNK_SIZE - 1) return (columnHeight == WorldConfig::CHUNK_COLUMN_HEIGHT - 1) ? Block::AIR : blocks[x][y + 1 + yOffset][z];
        return blocks[x][y + 1 + yOffset][z];
    };

    auto getNeighborBack = [&](int x, int y, int z) -> Block {
        if (z == 0) return blocks_Back ? blocks_Back[x][y + yOffset][WorldConfig::CHUNK_SIZE - 1] : Block::AIR;
        return blocks[x][y + yOffset][z - 1];
    };

    auto getNeighborFront = [&](int x, int y, int z) -> Block {
        if (z == WorldConfig::CHUNK_SIZE - 1) return blocks_Front ? blocks_Front[x][y + yOffset][0] : Block::AIR;
        return blocks[x][y + yOffset][z + 1];
    };

    auto shouldAddFace = [](Block currentBlock, Block neighborBlock) -> bool {
        if (isSolid(currentBlock)) {
            return neighborBlock == Block::AIR || isTransparent(neighborBlock);
        }
        if (isTransparent(currentBlock)) {
            return neighborBlock == Block::AIR;
        }
        return false;
    };
    
    for (int x = 0; x < WorldConfig::CHUNK_SIZE; x++){
        for (int y = 0; y < WorldConfig::CHUNK_SIZE; y++){
            for (int z = 0; z < WorldConfig::CHUNK_SIZE; z++){
                Block currentBlock = blocks[x][y + yOffset][z];
                if (currentBlock == Block::AIR) continue;

                std::vector<uint32_t>* targetVertices = nullptr;
                std::vector<uint16_t>* targetIndices = nullptr;

                if (isSolid(currentBlock)) {
                    targetVertices = &verticesOpaque;
                    targetIndices = &indiciesOpaque;
                } else if (isTransparent(currentBlock)) {
                    targetVertices = &verticesTransparent;
                    targetIndices = &indiciesTransparent;
                } else {
                    continue;
                }

                // LEFT
                Block leftNeighbor = getNeighborLeft(x, y, z);
                if (shouldAddFace(currentBlock, leftNeighbor)){
                    uint32_t idx = targetVertices->size();
                    targetVertices->emplace_back(create_vertex(x,y+1,z,getTextureID(currentBlock, Direction::LEFT), (int)Direction::LEFT, 0));   // TL
                    targetVertices->emplace_back(create_vertex(x,y+1,z+1,getTextureID(currentBlock, Direction::LEFT), (int)Direction::LEFT, 1)); // TR
                    targetVertices->emplace_back(create_vertex(x,y,z+1,getTextureID(currentBlock, Direction::LEFT), (int)Direction::LEFT, 2));   // BR
                    targetVertices->emplace_back(create_vertex(x,y,z,getTextureID(currentBlock, Direction::LEFT), (int)Direction::LEFT, 3));     // BL
                    
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 3); targetIndices->push_back(idx + 2);
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 2); targetIndices->push_back(idx + 1);
                }

                // RIGHT
                Block rightNeighbor = getNeighborRight(x, y, z);
                if (shouldAddFace(currentBlock, rightNeighbor)){
                    uint32_t idx = targetVertices->size();
                    targetVertices->emplace_back(create_vertex(x+1,y+1,z+1,getTextureID(currentBlock, Direction::RIGHT), (int)Direction::RIGHT, 0)); // TL
                    targetVertices->emplace_back(create_vertex(x+1,y+1,z,getTextureID(currentBlock, Direction::RIGHT), (int)Direction::RIGHT, 1));   // TR
                    targetVertices->emplace_back(create_vertex(x+1,y,z,getTextureID(currentBlock, Direction::RIGHT), (int)Direction::RIGHT, 2));     // BR
                    targetVertices->emplace_back(create_vertex(x+1,y,z+1,getTextureID(currentBlock, Direction::RIGHT), (int)Direction::RIGHT, 3));   // BL
                    
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 3); targetIndices->push_back(idx + 2);
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 2); targetIndices->push_back(idx + 1);
                }

                // DOWN
                Block downNeighbor = getNeighborDown(x, y, z);
                if (shouldAddFace(currentBlock, downNeighbor)){
                    uint32_t idx = targetVertices->size();
                    targetVertices->emplace_back(create_vertex(x,y,z+1,getTextureID(currentBlock, Direction::DOWN), (int)Direction::DOWN, 0));   // TL
                    targetVertices->emplace_back(create_vertex(x+1,y,z+1,getTextureID(currentBlock, Direction::DOWN), (int)Direction::DOWN, 1)); // TR
                    targetVertices->emplace_back(create_vertex(x+1,y,z,getTextureID(currentBlock, Direction::DOWN), (int)Direction::DOWN, 2));   // BR
                    targetVertices->emplace_back(create_vertex(x,y,z,getTextureID(currentBlock, Direction::DOWN), (int)Direction::DOWN, 3));     // BL
                    
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 3); targetIndices->push_back(idx + 2);
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 2); targetIndices->push_back(idx + 1);
                }

                // UP
                Block upNeighbor = getNeighborUp(x, y, z);
                if (shouldAddFace(currentBlock, upNeighbor)){
                    uint32_t idx = targetVertices->size();
                    targetVertices->emplace_back(create_vertex(x,y+1,z,getTextureID(currentBlock, Direction::UP), (int)Direction::UP, 0));     // TL
                    targetVertices->emplace_back(create_vertex(x+1,y+1,z,getTextureID(currentBlock, Direction::UP), (int)Direction::UP, 1));   // TR
                    targetVertices->emplace_back(create_vertex(x+1,y+1,z+1,getTextureID(currentBlock, Direction::UP), (int)Direction::UP, 2)); // BR
                    targetVertices->emplace_back(create_vertex(x,y+1,z+1,getTextureID(currentBlock, Direction::UP), (int)Direction::UP, 3));   // BL
                    
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 3); targetIndices->push_back(idx + 2);
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 2); targetIndices->push_back(idx + 1);
                }

                // BACKWARD
                Block backNeighbor = getNeighborBack(x, y, z);
                if (shouldAddFace(currentBlock, backNeighbor)){
                    uint32_t idx = targetVertices->size();
                    targetVertices->emplace_back(create_vertex(x+1,y+1,z,getTextureID(currentBlock, Direction::BACKWARD), (int)Direction::BACKWARD, 0)); // TL
                    targetVertices->emplace_back(create_vertex(x,y+1,z,getTextureID(currentBlock, Direction::BACKWARD), (int)Direction::BACKWARD, 1));   // TR
                    targetVertices->emplace_back(create_vertex(x,y,z,getTextureID(currentBlock, Direction::BACKWARD), (int)Direction::BACKWARD, 2));     // BR
                    targetVertices->emplace_back(create_vertex(x+1,y,z,getTextureID(currentBlock, Direction::BACKWARD), (int)Direction::BACKWARD, 3));   // BL
                    
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 3); targetIndices->push_back(idx + 2);
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 2); targetIndices->push_back(idx + 1);
                }

                // FORWARD
                Block frontNeighbor = getNeighborFront(x, y, z);
                if (shouldAddFace(currentBlock, frontNeighbor)){
                    uint32_t idx = targetVertices->size();
                    targetVertices->emplace_back(create_vertex(x,y+1,z+1,getTextureID(currentBlock, Direction::FORWARD), (int)Direction::FORWARD, 0)); // TL
                    targetVertices->emplace_back(create_vertex(x+1,y+1,z+1,getTextureID(currentBlock, Direction::FORWARD), (int)Direction::FORWARD, 1)); // TR
                    targetVertices->emplace_back(create_vertex(x+1,y,z+1,getTextureID(currentBlock, Direction::FORWARD), (int)Direction::FORWARD, 2));   // BR
                    targetVertices->emplace_back(create_vertex(x,y,z+1,getTextureID(currentBlock, Direction::FORWARD), (int)Direction::FORWARD, 3));     // BL
                    
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 3); targetIndices->push_back(idx + 2);
                    targetIndices->push_back(idx); targetIndices->push_back(idx + 2); targetIndices->push_back(idx + 1);
                }
            }
        }
    }
    indiciesCountOpaque = indiciesOpaque.size();
    verticesCountOpaque = verticesOpaque.size();

    if (verticesCountOpaque > 0) {
        if (chunkAllocationOpaque.vertexCount < verticesOpaque.size() || chunkAllocationOpaque.indexCount < indiciesOpaque.size()){
            if (chunkAllocationOpaque.vertexBlocks > 0 && chunkAllocationOpaque.indexBlocks > 0){
                vramAllocator->free(chunkAllocationOpaque);
            }
            chunkAllocationOpaque = vramAllocator->allocate(verticesCountOpaque, indiciesCountOpaque);
        }
        glNamedBufferSubData(vramAllocator->getVertexBufferID(), chunkAllocationOpaque.vertexBlocks * sizeof(uint32_t), verticesCountOpaque * sizeof(uint32_t), verticesOpaque.data());
        glNamedBufferSubData(vramAllocator->getIndexBufferID(), chunkAllocationOpaque.indexBlocks * sizeof(uint16_t), indiciesCountOpaque * sizeof(uint16_t), indiciesOpaque.data());
    }

    indiciesCountTransparent = indiciesTransparent.size();
    verticesCountTransparent = verticesTransparent.size();

    if (verticesCountTransparent > 0) {
        if (chunkAllocationTransparent.vertexCount < verticesTransparent.size() || chunkAllocationTransparent.indexCount < indiciesTransparent.size()){
            if (chunkAllocationTransparent.vertexBlocks > 0 && chunkAllocationTransparent.indexBlocks > 0){
                vramAllocator->free(chunkAllocationTransparent);
            }
            chunkAllocationTransparent = vramAllocator->allocate(verticesCountTransparent, indiciesCountTransparent);
        }
        glNamedBufferSubData(vramAllocator->getVertexBufferID(), chunkAllocationTransparent.vertexBlocks * sizeof(uint32_t), verticesCountTransparent * sizeof(uint32_t), verticesTransparent.data());
        glNamedBufferSubData(vramAllocator->getIndexBufferID(), chunkAllocationTransparent.indexBlocks * sizeof(uint16_t), indiciesCountTransparent * sizeof(uint16_t), indiciesTransparent.data());
    }

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