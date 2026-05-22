#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <Direction.hpp>

constexpr int CHUNK_SIZE = 16;
// expand to 16 later, for now we can just use 4 to test the chunk column rendering
constexpr int CHUNK_COLUMN_HEIGHT = 4;

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

Chunk create_chunk(glm::ivec3 chunkPosition);
inline uint32_t create_vertex(int x, int y, int z, int TextureID, int Direction) {
    /*
        x - 5 bits
        y - 8 bits
        z - 5 bits
        TextureID - 8 bits
        Direction - 3 bits
    */
    return ((x & 0x1F) | ((y & 0xFF) << 5) | ((z & 0x1F) << 13) | ((TextureID & 0xFF) << 18) | ((Direction & 0x7) << 26));
}



class ChunkColumn{
    public:
    
    ChunkColumn(glm::vec2 position);
    ~ChunkColumn();

    ChunkColumn(const ChunkColumn&) = delete;
    ChunkColumn& operator=(const ChunkColumn&) = delete;

    ChunkColumn(ChunkColumn&& other) noexcept;
    ChunkColumn& operator=(ChunkColumn&& other) noexcept;

    uint8_t Blocks[CHUNK_SIZE][CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE];
    Chunk chunks[CHUNK_COLUMN_HEIGHT];
    glm::vec2 position;


    private:

};