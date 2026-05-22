#include "Chunk.hpp"
#include <glad/glad.h>
#include <cstring>

Chunk::Chunk() {
}

Chunk::Chunk(Chunk&& other) noexcept 
    : vao(other.vao), vbo(other.vbo), ebo(other.ebo), indiciesCount(other.indiciesCount), verticesCount(other.verticesCount) {
    
    other.vao = 0;
    other.vbo = 0;
    other.ebo = 0;
    other.indiciesCount = 0;
    other.verticesCount = 0;
}

Chunk& Chunk::operator=(Chunk&& other) noexcept {
    if (this != &other) {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (ebo != 0) glDeleteBuffers(1, &ebo);

        vao = other.vao;
        vbo = other.vbo;
        ebo = other.ebo;
        indiciesCount = other.indiciesCount;
        verticesCount = other.verticesCount;

        other.vao = 0;
        other.vbo = 0;
        other.ebo = 0;
        other.indiciesCount = 0;
        other.verticesCount = 0;
    }
    return *this;
}

void Chunk::generateMesh(uint8_t (&Blocks)[CHUNK_SIZE][CHUNK_SIZE * CHUNK_COLUMN_HEIGHT][CHUNK_SIZE], int columnHeight) {

    std::vector<uint32_t> vertices;
    std::vector<uint16_t> indicies;

    int yOffset = columnHeight * CHUNK_SIZE;

    for (int x = 0; x < CHUNK_SIZE; x++){
        for (int y = 0; y < CHUNK_SIZE; y++){
            for (int z = 0; z < CHUNK_SIZE; z++){
                if (Blocks[x][y + yOffset][z] == 0) continue;
                if (x == 0 || Blocks[x-1][y + yOffset][z] == 0){ // LEFT
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::LEFT));   // TL
                    vertices.emplace_back(create_vertex(x,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::LEFT)); // TR
                    vertices.emplace_back(create_vertex(x,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::LEFT));   // BR
                    vertices.emplace_back(create_vertex(x,y,z,Blocks[x][y + yOffset][z], (int)Direction::LEFT));     // BL
                    
                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if (x == CHUNK_SIZE-1 || Blocks[x+1][y + yOffset][z] == 0){ // RIGHT
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x+1,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::RIGHT)); // TL
                    vertices.emplace_back(create_vertex(x+1,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::RIGHT));   // TR
                    vertices.emplace_back(create_vertex(x+1,y,z,Blocks[x][y + yOffset][z], (int)Direction::RIGHT));     // BR
                    vertices.emplace_back(create_vertex(x+1,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::RIGHT));   // BL

                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if (y == 0 || Blocks[x][y-1 + yOffset][z] == 0){ // DOWN
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::DOWN));   // TL
                    vertices.emplace_back(create_vertex(x+1,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::DOWN)); // TR
                    vertices.emplace_back(create_vertex(x+1,y,z,Blocks[x][y + yOffset][z], (int)Direction::DOWN));   // BR
                    vertices.emplace_back(create_vertex(x,y,z,Blocks[x][y + yOffset][z], (int)Direction::DOWN));     // BL

                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if (y == CHUNK_SIZE-1 || Blocks[x][y+1 + yOffset][z] == 0){ // UP
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::UP));     // TL
                    vertices.emplace_back(create_vertex(x+1,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::UP));   // TR
                    vertices.emplace_back(create_vertex(x+1,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::UP)); // BR
                    vertices.emplace_back(create_vertex(x,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::UP));   // BL

                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if (z == 0 || Blocks[x][y + yOffset][z-1] == 0){ // BACKWARD
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x+1,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::BACKWARD)); // TL
                    vertices.emplace_back(create_vertex(x,y+1,z,Blocks[x][y + yOffset][z], (int)Direction::BACKWARD));   // TR
                    vertices.emplace_back(create_vertex(x,y,z,Blocks[x][y + yOffset][z], (int)Direction::BACKWARD));     // BR
                    vertices.emplace_back(create_vertex(x+1,y,z,Blocks[x][y + yOffset][z], (int)Direction::BACKWARD));   // BL

                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
                if (z == CHUNK_SIZE-1 || Blocks[x][y + yOffset][z+1] == 0){ // FORWARD
                    uint32_t idx = vertices.size();
                    vertices.emplace_back(create_vertex(x,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::FORWARD)); // TL
                    vertices.emplace_back(create_vertex(x+1,y+1,z+1,Blocks[x][y + yOffset][z], (int)Direction::FORWARD)); // TR
                    vertices.emplace_back(create_vertex(x+1,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::FORWARD));   // BR
                    vertices.emplace_back(create_vertex(x,y,z+1,Blocks[x][y + yOffset][z], (int)Direction::FORWARD));     // BL

                    indicies.push_back(idx); indicies.push_back(idx + 3); indicies.push_back(idx + 2);
                    indicies.push_back(idx); indicies.push_back(idx + 2); indicies.push_back(idx + 1);
                }
            }
        }
    }
    indiciesCount = indicies.size();
    verticesCount = vertices.size();

    if (indiciesCount == 0) return;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glCreateBuffers(1, &vbo);
    glCreateBuffers(1, &ebo);

    glNamedBufferData(vbo, vertices.size() * sizeof(uint32_t), vertices.data(), GL_STATIC_DRAW);
    glNamedBufferData(ebo, indicies.size() * sizeof(uint16_t), indicies.data(), GL_STATIC_DRAW);
    
    glVertexArrayElementBuffer(vao, ebo);

    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribIFormat(vao, 0, 1, GL_UNSIGNED_INT, 0);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(uint32_t));
    glVertexArrayAttribBinding(vao, 0, 0);

    glBindVertexArray(0);

}

Chunk::~Chunk(){
    if (vao != 0) glDeleteVertexArrays(1, &vao);
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (ebo != 0) glDeleteBuffers(1, &ebo);
}

ChunkColumn::ChunkColumn(glm::vec2 position) 
    : position(position)
{
    std::memset(Blocks, 0, sizeof(Blocks));
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