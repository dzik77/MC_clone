#include "Chunk.hpp"
#include <glad/glad.h>

Chunk create_chunk(glm::ivec3 chunkPosition) {
    // for now just a 16x16 Giant Block
    Chunk chunk(chunkPosition);
    for (int x = 0; x < CHUNK_SIZE; x++){
        for (int y = 0; y < CHUNK_SIZE; y++){
            for (int z = 0; z < CHUNK_SIZE; z++){
                chunk.Data[x][y][z] = 0;
            }
        }
    }
    for (int x = 0; x < CHUNK_SIZE; x++){
        for (int z = 0; z < CHUNK_SIZE; z++){
            chunk.Data[x][0][z] = 1;
        }
    }
    for (int x = 0; x < CHUNK_SIZE; x++){
        for (int y = 0; y < CHUNK_SIZE; y++){
            for (int z = 0; z < CHUNK_SIZE; z++){
                if (x == 0){ // RIGHT
                    uint16_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z,chunk.Data[x][y][z], (int)Direction::RIGHT));
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z+1,chunk.Data[x][y][z], (int)Direction::RIGHT));
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z,chunk.Data[x][y][z], (int)Direction::RIGHT));
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z+1,chunk.Data[x][y][z], (int)Direction::RIGHT));
                    std::vector<uint16_t> append_indicies = {idx, static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 3)};
                    chunk.indicies.insert(chunk.indicies.end(), append_indicies.begin(), append_indicies.end());
                }
                if (x == CHUNK_SIZE-1){ // LEFT
                    uint16_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x,y,z,chunk.Data[x][y][z], (int)Direction::LEFT));
                    chunk.vertices.emplace_back(create_vertex(x,y,z+1,chunk.Data[x][y][z], (int)Direction::LEFT));
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z,chunk.Data[x][y][z], (int)Direction::LEFT));
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z+1,chunk.Data[x][y][z], (int)Direction::LEFT));
                    std::vector<uint16_t> append_indicies = {idx, static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 3)};
                    chunk.indicies.insert(chunk.indicies.end(), append_indicies.begin(), append_indicies.end());
                }
                if (y == 0){ // UP
                    uint16_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z,chunk.Data[x][y][z], (int)Direction::UP));
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z,chunk.Data[x][y][z], (int)Direction::UP));
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z+1,chunk.Data[x][y][z], (int)Direction::UP));
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z+1,chunk.Data[x][y][z], (int)Direction::UP));
                    std::vector<uint16_t> append_indicies = {idx, static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 3)};
                    chunk.indicies.insert(chunk.indicies.end(), append_indicies.begin(), append_indicies.end());
                }
                if (y == CHUNK_SIZE-1){ // DOWN
                    uint16_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x,y,z,chunk.Data[x][y][z], (int)Direction::DOWN));
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z,chunk.Data[x][y][z], (int)Direction::DOWN));
                    chunk.vertices.emplace_back(create_vertex(x,y,z+1,chunk.Data[x][y][z], (int)Direction::DOWN));
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z+1,chunk.Data[x][y][z], (int)Direction::DOWN));
                    std::vector<uint16_t> append_indicies = {idx, static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 3)};
                    chunk.indicies.insert(chunk.indicies.end(), append_indicies.begin(), append_indicies.end());
                }
                if (z == 0){ // FORWARD
                    uint16_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x,y,z+1,chunk.Data[x][y][z], (int)Direction::FORWARD));
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z+1,chunk.Data[x][y][z], (int)Direction::FORWARD));
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z+1,chunk.Data[x][y][z], (int)Direction::FORWARD));
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z+1,chunk.Data[x][y][z], (int)Direction::FORWARD));
                    std::vector<uint16_t> append_indicies = {idx, static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 3)};
                    chunk.indicies.insert(chunk.indicies.end(), append_indicies.begin(), append_indicies.end());
                }
                if (z == CHUNK_SIZE-1){ // BACKWARD
                    uint16_t idx = chunk.vertices.size();
                    chunk.vertices.emplace_back(create_vertex(x,y,z,chunk.Data[x][y][z], (int)Direction::BACKWARD));
                    chunk.vertices.emplace_back(create_vertex(x+1,y,z,chunk.Data[x][y][z], (int)Direction::BACKWARD));
                    chunk.vertices.emplace_back(create_vertex(x,y+1,z,chunk.Data[x][y][z], (int)Direction::BACKWARD));
                    chunk.vertices.emplace_back(create_vertex(x+1,y+1,z,chunk.Data[x][y][z], (int)Direction::BACKWARD));
                    std::vector<uint16_t> append_indicies = {idx, static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 2), static_cast<uint16_t>(idx + 1), static_cast<uint16_t>(idx + 3)};
                    chunk.indicies.insert(chunk.indicies.end(), append_indicies.begin(), append_indicies.end());
                }
            }
        }
    }

    glGenVertexArrays(1, &chunk.vao);
    glGenBuffers(1, &chunk.vbo);
    glGenBuffers(1, &chunk.ebo);
    glBindVertexArray(chunk.vao);
    glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(chunk.vertices), &chunk.vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(unsigned int), (void *) 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(chunk.indicies), &chunk.indicies, GL_STATIC_DRAW);

    return chunk;
}

Chunk::Chunk(glm::vec3 position) : position(position) {
    
}

Chunk::~Chunk(){
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}
