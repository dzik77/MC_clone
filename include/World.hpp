#pragma once
#include <glm/glm.hpp>
#include <Chunk.hpp>
#include <vector>
#include <Shaders.hpp>

class World{
    public:
        World();
        void loadChunks(glm::vec3 playerPosition);
        void render(Shader& shader);
        ~World();

    private:

        int renderDistance = 8;
        std::vector<Chunk> Chunks; // sorted by pairs {x,y};

};