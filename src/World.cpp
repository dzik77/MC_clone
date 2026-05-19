#include "World.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

const std::vector<std::filesystem::path> texturePaths = {"textures/stone.png","textures/dirt.png","textures/grass_block_side.png","textures/grass_block_top_item.png" };


World::World() {
}

void World::loadTextures() {
    textureArray.createTextureArray(texturePaths,16,16);
}

void World::loadChunks(glm::vec3 playerPosition) {
    // TODO:
    // Load only unloaded chunks
    glm::ivec3 chunkPosition(playerPosition.x / CHUNK_SIZE, playerPosition.y / CHUNK_SIZE, playerPosition.z / CHUNK_SIZE);
    std::vector<Chunk> newChunks;
    int pt2 = 0;
    for (int x = -renderDistance; x <= renderDistance; x++){
        for (int z = -renderDistance; z <= renderDistance; z++){
            if(x*x + z*z > renderDistance*renderDistance) 
                continue;
            while(pt2 < Chunks.size() && std::pair<int,int>(Chunks[pt2].position.x,Chunks[pt2].position.z) < std::pair<int,int>(chunkPosition.x+x,chunkPosition.z+z))
                pt2++;
            if (pt2 < Chunks.size() && std::pair<int,int>(Chunks[pt2].position.x,Chunks[pt2].position.z) == std::pair<int,int>(chunkPosition.x+x,chunkPosition.z+z)){
                newChunks.push_back(std::move(Chunks[pt2]));
                pt2++;
            } else {
                newChunks.push_back(create_chunk(glm::ivec3(chunkPosition.x+x,0,chunkPosition.z+z)));
            }
        }
    }
    Chunks = std::move(newChunks);
}

void World::render(Shader& shader) {

    shader.use();
    glBindTextureUnit(0, textureArray.texture); 
    for (int i = 0; i < Chunks.size(); i++){
        glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(Chunks[i].position * CHUNK_SIZE));
        shader.setMat4("model", model);
        glBindVertexArray(Chunks[i].vao);
        glDrawElements(GL_TRIANGLES, Chunks[i].indicies.size(), GL_UNSIGNED_SHORT, 0);
    }

}

void World::clean(){
    Chunks.clear();
    textureArray.clean();
}

World::~World() {
    Chunks.clear();
}
