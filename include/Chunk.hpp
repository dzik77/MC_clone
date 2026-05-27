#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <Direction.hpp>
#include <VramAllocator.hpp>
#include <utility>

struct ChunkPosition{
    glm::vec4 position;
};

constexpr int CHUNK_SIZE = 16;
// expand to 16 later, for now we can just use 4 to test the chunk column rendering
constexpr int CHUNK_COLUMN_HEIGHT = 4;

#ifndef NOT_AZDO_RENDERING
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
        uint8_t (*Blocks)[CHUNK_SIZE][CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE], int columnHeight,
        uint8_t (*Blocks_Front)[CHUNK_SIZE][CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE],
        uint8_t (*Blocks_Back)[CHUNK_SIZE][CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE],
        uint8_t (*Blocks_Left)[CHUNK_SIZE][CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE],
        uint8_t (*Blocks_Right)[CHUNK_SIZE][CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE]);   
int verticesCount = 0;
    int indiciesCount = 0;
    uint32_t ssboIndex = -1;
    VramAllocator* vramAllocator;
    BufferAllocation chunkAllocation;

    private:
};
#else
class Chunk{
    public:
    Chunk();
    ~Chunk();

    // Delete copy semantics to prevent accidental shallow copies
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    // Declare move semantics
    Chunk(Chunk&& other) noexcept;
    Chunk& operator=(Chunk&& other) noexcept;

    void generateMesh(uint8_t (&Blocks)[CHUNK_SIZE][CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE], int columnHeight);

    unsigned int vao = 0, vbo = 0, ebo = 0;
    int indiciesCount = 0;
    int verticesCount = 0;
    private:
};
#endif

Chunk create_chunk(glm::ivec3 chunkPosition);
inline uint32_t create_vertex(int x, int y, int z, int TextureID, int Direction, int UVCoordinate) {
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

    uint8_t Blocks[CHUNK_SIZE][CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE];
    Chunk chunks[CHUNK_COLUMN_HEIGHT];
    glm::vec2 position;
    VramAllocator* vramAllocator;

    private:
};