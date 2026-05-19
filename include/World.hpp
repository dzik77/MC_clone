#pragma once
#include <glm/glm.hpp>
#include <Chunk.hpp>
#include <vector>
#include <Shaders.hpp>
#include <TextureManager.hpp>

class World{
    public:
        World();
        void loadTextures();
        void loadChunks(glm::vec3 playerPosition);
        void render(Shader& shader);
        void clean();
        ~World();

    private:

        int renderDistance = 32;
        std::vector<Chunk> Chunks;
        TextureArray textureArray;

};