#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <WorldConfig.hpp>
#include <Direction.hpp>
#include <Engine/VramAllocator.hpp>
#include <utility>
#include <Block.hpp>

struct ChunkPosition{
    glm::vec4 position;
};


class Chunk{
    public:
    Chunk();
    Chunk(VramAllocator* vramAllocator);
    ~Chunk();

    void init(VramAllocator* vramAllocator);

    // Delete copy semantics to prevent accidental shallow copies
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    // Declare move semantics
    Chunk(Chunk&& other) noexcept;
    Chunk& operator=(Chunk&& other) noexcept;

    void generateMesh(
        Block (*blocks)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE], int columnHeight,
        Block (*blocks_Front)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE],
        Block (*blocks_Back)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE],
        Block (*blocks_Left)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE],
        Block (*blocks_Right)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE]);   
    int verticesCount = 0;
    int indiciesCount = 0;
    uint32_t ssboIndex = -1;
    VramAllocator* vramAllocator;
    BufferAllocation chunkAllocation;

    private:
};

Chunk create_chunk(glm::ivec3 chunkPosition);
inline uint32_t create_vertex(int x, int y, int z, uint8_t TextureID, int Direction, int UVCoordinate) {
    /*
        x - 5 bits
        y - 5 bits
        z - 5 bits
        TextureID - 8 bits
        Direction - 3 bits
        UVCoordinate - 2bits
    */
    return ((x & 0x1F) | ((y & 0x1F) << 5) | ((z & 0x1F) << 10) | ((TextureID & 0xFF) << 15) | ((Direction & 0x7) << 23) | ((UVCoordinate & 0x3) << 26));
}



class ChunkColumn{
    public:
    
    ChunkColumn(glm::vec2 position, VramAllocator* vramAllocator);
    ~ChunkColumn();

    ChunkColumn(const ChunkColumn&) = delete;
    ChunkColumn& operator=(const ChunkColumn&) = delete;

    ChunkColumn(ChunkColumn&& other) noexcept;
    ChunkColumn& operator=(ChunkColumn&& other) noexcept;

    Block blocks[WorldConfig::CHUNK_SIZE][WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE];
    Chunk chunks[WorldConfig::CHUNK_COLUMN_HEIGHT];
    glm::vec2 position;
    VramAllocator* vramAllocator;

    private:
};