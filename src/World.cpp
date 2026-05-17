#include "World.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

World::World() {

}

void World::loadChunks(glm::vec3 playerPosition) {
    // TODO:
    // Load only unloaded chunks
    glm::ivec3 chunkPosition(playerPosition.x / CHUNK_SIZE, playerPosition.y / CHUNK_SIZE, playerPosition.z / CHUNK_SIZE);
    std::vector<Chunk> newChunks;
    for (int x = -renderDistance; x <= renderDistance; x++){
        for (int z = -renderDistance; z <= renderDistance; z++){
            if(x*x + z*z > renderDistance*renderDistance) 
                continue;
            newChunks.push_back(create_chunk(glm::ivec3(chunkPosition.x+x,chunkPosition.y,chunkPosition.z+z)));
        }
    }
    swap(newChunks, Chunks);
}

void World::render(Shader& shader) {

    shader.use();
    for (auto& chunk:Chunks){
        glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(chunk.position * CHUNK_SIZE));
        shader.setMat4("model", model);
        glDrawElements(GL_TRIANGLES, chunk.indicies.size(), GL_UNSIGNED_SHORT, 0);
    }

}

World::~World() {

}
