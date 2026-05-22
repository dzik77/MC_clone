#include "World.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

const std::vector<std::filesystem::path> texturePaths = {"textures/stone.png","textures/dirt.png","textures/grass_block_side.png","textures/grass_block_top_item.png" };


World::World() {

    noise = FastNoise::New<FastNoise::Simplex>();

}

void World::loadTextures() {
    textureArray.createTextureArray(texturePaths,16,16);
}

void World::generateTerrain(ChunkColumn &chunkColumn) {
    std::vector<float> chunkBuffer(CHUNK_SIZE * CHUNK_SIZE);
    noise->GenUniformGrid2D(chunkBuffer.data(), chunkColumn.position.x * CHUNK_SIZE, chunkColumn.position.y * CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE, 1.f, 1.f, 67);
    for (int x = 0; x < CHUNK_SIZE; x++){
        for (int z = 0; z < CHUNK_SIZE; z++){
            int height = std::max(int(floor((chunkBuffer[x + z * CHUNK_SIZE] + 1.f) / 2.f * (float)CHUNK_SIZE) * (float)CHUNK_COLUMN_HEIGHT),1);
            for (int y = 0; y < height; y++){
                chunkColumn.Blocks[x][y][z] = 4;
            }
        }
    }
    for (int i = 0; i < CHUNK_COLUMN_HEIGHT; i++){
        chunkColumn.chunks[i].generateMesh(chunkColumn.Blocks, i);
    }
}

void World::loadChunks(glm::vec3 playerPosition) {
    
    glm::ivec3 chunkPosition(playerPosition.x / CHUNK_SIZE, 0, playerPosition.z / CHUNK_SIZE);
    int pt2 = 0;
    std::vector<int> toRemove;
    for (int i = 0; i < ChunkColumns.size(); i++){
        if ( (ChunkColumns[i].position.x-chunkPosition.x) * (ChunkColumns[i].position.x-chunkPosition.x) + (ChunkColumns[i].position.y-chunkPosition.z) * (ChunkColumns[i].position.y-chunkPosition.z) > renderDistance*renderDistance){
            toRemove.push_back(i);
        }
    }
    for (int x = -renderDistance; x <= renderDistance; x++){
        for (int z = -renderDistance; z <= renderDistance; z++){
            if(x*x + z*z > renderDistance*renderDistance) 
                continue;
            if (loadedChunkPositions.find(std::pair<int,int>(chunkPosition.x + x, chunkPosition.z + z)) == loadedChunkPositions.end()){
                if (toRemove.size() > 0){
                    int idx = toRemove.back();
                    toRemove.pop_back();
                    loadedChunkPositions.erase(std::pair<int,int>(ChunkColumns[idx].position.x, ChunkColumns[idx].position.y));
                    ChunkColumns[idx] = ChunkColumn(glm::vec2(chunkPosition.x + x, chunkPosition.z + z));
                    generateTerrain(ChunkColumns[idx]);
                    loadedChunkPositions.insert(std::pair<int,int>(chunkPosition.x + x, chunkPosition.z + z));
                } else {
                    ChunkColumns.emplace_back(glm::vec2(chunkPosition.x + x, chunkPosition.z + z));
                    generateTerrain(ChunkColumns.back());
                    loadedChunkPositions.insert(std::pair<int,int>(chunkPosition.x + x, chunkPosition.z + z));
                }
            }
        }
    }
}

void World::render(Shader& shader) {


    shader.use();
    glBindTextureUnit(0, textureArray.texture); 
    for (int i = 0; i < ChunkColumns.size(); i++){
        for (int j = 0; j < CHUNK_COLUMN_HEIGHT; j++){
            if (ChunkColumns[i].chunks[j].vao == 0 || ChunkColumns[i].chunks[j].indiciesCount == 0) continue;
            glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(ChunkColumns[i].position.x * CHUNK_SIZE, j * CHUNK_SIZE, ChunkColumns[i].position.y * CHUNK_SIZE));
            shader.setMat4("model", model);
            glBindVertexArray(ChunkColumns[i].chunks[j].vao);
            glDrawElements(GL_TRIANGLES, ChunkColumns[i].chunks[j].indiciesCount, GL_UNSIGNED_SHORT, 0);
        }
    }

}

void World::clean(){
    ChunkColumns.clear();
    textureArray.clean();
}

World::~World() {
    ChunkColumns.clear();
}
