#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <Direction.hpp>

constexpr int CHUNK_SIZE = 16;

class Chunk{
    public:
    Chunk(glm::vec3 position);
    ~Chunk();

    // Delete copy semantics to prevent accidental shallow copies
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    // Declare move semantics
    Chunk(Chunk&& other) noexcept;
    Chunk& operator=(Chunk&& other) noexcept;

    uint8_t Data[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    std::vector<uint32_t> vertices;
    std::vector<uint16_t> indicies;
    unsigned int vao = 0, vbo = 0, ebo = 0;
    glm::ivec3 position; 
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