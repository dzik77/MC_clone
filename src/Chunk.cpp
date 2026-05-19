#include "Chunk.hpp"
#include <glad/glad.h>
#include <cstring>

Chunk create_chunk(glm::ivec3 chunkPosition) {
    // for now just a 16x16 Giant Block
    Chunk chunk(chunkPosition);
    for (int x = 0; x < CHUNK_SIZE; x++){
        for (int y = 0; y < CHUNK_SIZE; y++){
            for (int z = 0; z < CHUNK_SIZE; z++){
                chunk.Data[x][y][z] = 1;
            }
        }
    }
    for (int x = 0; x < CHUNK_SIZE; x++){
        for (int z = 0; z < CHUNK_SIZE; z++){
            chunk.Data[x][15][z] = 4;
        }
    }
    for (int x = 0; x < CHUNK_SIZE; x++){
        for (int y = 0; y < CHUNK_SIZE; y++){
            for (int z = 0; z < CHUNK_SIZE; z++){
                if (x == 0){ // LEFT
                    uint32_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z,chunk.Data[x][y][z], (int)Direction::LEFT));   // TL
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z+1,chunk.Data[x][y][z], (int)Direction::LEFT)); // TR
                    chunk.vertices.emplace_back(create_vertex(x,y,z+1,chunk.Data[x][y][z], (int)Direction::LEFT));   // BR
                    chunk.vertices.emplace_back(create_vertex(x,y,z,chunk.Data[x][y][z], (int)Direction::LEFT));     // BL
                    
                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 3); chunk.indicies.push_back(idx + 2);
                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 2); chunk.indicies.push_back(idx + 1);
                }
                if (x == CHUNK_SIZE-1){ // RIGHT
                    uint32_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z+1,chunk.Data[x][y][z], (int)Direction::RIGHT)); // TL
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z,chunk.Data[x][y][z], (int)Direction::RIGHT));   // TR
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z,chunk.Data[x][y][z], (int)Direction::RIGHT));     // BR
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z+1,chunk.Data[x][y][z], (int)Direction::RIGHT));   // BL

                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 3); chunk.indicies.push_back(idx + 2);
                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 2); chunk.indicies.push_back(idx + 1);
                }
                if (y == 0){ // DOWN
                    uint32_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x,y,z+1,chunk.Data[x][y][z], (int)Direction::DOWN));   // TL
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z+1,chunk.Data[x][y][z], (int)Direction::DOWN)); // TR
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z,chunk.Data[x][y][z], (int)Direction::DOWN));   // BR
                    chunk.vertices.emplace_back(create_vertex(x,y,z,chunk.Data[x][y][z], (int)Direction::DOWN));     // BL

                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 3); chunk.indicies.push_back(idx + 2);
                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 2); chunk.indicies.push_back(idx + 1);
                }
                if (y == CHUNK_SIZE-1){ // UP
                    uint32_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z,chunk.Data[x][y][z], (int)Direction::UP));     // TL
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z,chunk.Data[x][y][z], (int)Direction::UP));   // TR
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z+1,chunk.Data[x][y][z], (int)Direction::UP)); // BR
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z+1,chunk.Data[x][y][z], (int)Direction::UP));   // BL

                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 3); chunk.indicies.push_back(idx + 2);
                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 2); chunk.indicies.push_back(idx + 1);
                }
                if (z == 0){ // BACKWARD
                    uint32_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z,chunk.Data[x][y][z], (int)Direction::BACKWARD)); // TL
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z,chunk.Data[x][y][z], (int)Direction::BACKWARD));   // TR
                    chunk.vertices.emplace_back(create_vertex(x,y,z,chunk.Data[x][y][z], (int)Direction::BACKWARD));     // BR
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z,chunk.Data[x][y][z], (int)Direction::BACKWARD));   // BL

                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 3); chunk.indicies.push_back(idx + 2);
                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 2); chunk.indicies.push_back(idx + 1);
                }
                if (z == CHUNK_SIZE-1){ // FORWARD
                    uint32_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z+1,chunk.Data[x][y][z], (int)Direction::FORWARD)); // TL
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z+1,chunk.Data[x][y][z], (int)Direction::FORWARD)); // TR
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z+1,chunk.Data[x][y][z], (int)Direction::FORWARD));   // BR
                    chunk.vertices.emplace_back(create_vertex(x,y,z+1,chunk.Data[x][y][z], (int)Direction::FORWARD));     // BL

                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 3); chunk.indicies.push_back(idx + 2);
                    chunk.indicies.push_back(idx); chunk.indicies.push_back(idx + 2); chunk.indicies.push_back(idx + 1);
                }
            }
        }
    }

    glCreateVertexArrays(1, &chunk.vao);
    glCreateBuffers(1, &chunk.vbo);
    glCreateBuffers(1, &chunk.ebo);

    glNamedBufferData(chunk.vbo, chunk.vertices.size() * sizeof(uint32_t), chunk.vertices.data(), GL_STATIC_DRAW);
    glNamedBufferData(chunk.ebo, chunk.indicies.size() * sizeof(uint16_t), chunk.indicies.data(), GL_STATIC_DRAW);
    
    glVertexArrayElementBuffer(chunk.vao, chunk.ebo);

    glEnableVertexArrayAttrib(chunk.vao, 0);
    glVertexArrayAttribIFormat(chunk.vao, 0, 1, GL_UNSIGNED_INT, 0);

    glVertexArrayVertexBuffer(chunk.vao, 0, chunk.vbo, 0, sizeof(uint32_t));
    glVertexArrayAttribBinding(chunk.vao, 0, 0);

    return chunk;
}

Chunk::Chunk(glm::vec3 position) : position(position) {
    
}

Chunk::Chunk(Chunk&& other) noexcept 
    : position(other.position), 
      vertices(std::move(other.vertices)), 
      indicies(std::move(other.indicies)),
      vao(other.vao), vbo(other.vbo), ebo(other.ebo) {
    
    std::memcpy(Data, other.Data, sizeof(Data));

    other.vao = 0;
    other.vbo = 0;
    other.ebo = 0;
}

Chunk& Chunk::operator=(Chunk&& other) noexcept {
    if (this != &other) {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (ebo != 0) glDeleteBuffers(1, &ebo);

        position = other.position;
        vertices = std::move(other.vertices);
        indicies = std::move(other.indicies);
        vao = other.vao;
        vbo = other.vbo;
        ebo = other.ebo;
        std::memcpy(Data, other.Data, sizeof(Data));

        other.vao = 0;
        other.vbo = 0;
        other.ebo = 0;
    }
    return *this;
}

Chunk::~Chunk(){
    if (vao != 0) glDeleteVertexArrays(1, &vao);
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (ebo != 0) glDeleteBuffers(1, &ebo);
}
