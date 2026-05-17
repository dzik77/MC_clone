#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <Direction.hpp>

constexpr int CHUNK_SIZE = 16;

class Chunk{
    public:
    Chunk(glm::vec3 position);
    ~Chunk();
    uint8_t Data[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    std::vector<uint32_t> vertices;
    std::vector<uint16_t> indicies;
    unsigned int vao,vbo,ebo;
    glm::ivec3 position; 
    private:
};

Chunk create_chunk(glm::ivec3 chunkPosition);
inline uint32_t create_vertex(int x, int y, int z, int TextureID, int Direction) {
    /*
        x - 4bits
        z - 4bits
        y - 8bits
        TextureID - 8bits
        Direction - 3bits
    */
    return (x | (z << 4) | (y << 8) | (TextureID << 16) | (Direction << 24));
}