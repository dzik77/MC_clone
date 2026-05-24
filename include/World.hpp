#pragma once
#include <glm/glm.hpp>
#include <Chunk.hpp>
#include <vector>
#include <Shaders.hpp>
#include <TextureManager.hpp>
#include <FastNoise/FastNoise.h>
#include <set>

class World{
    public:
    
        World();
        void loadTextures();
        void generateTerrain(ChunkColumn &chunkColumn);
        void loadChunks(glm::vec3 playerPosition);
        void render(Shader& shader);
        void clean();
        ~World();

    private:

        int renderDistance = 32;
        std::vector<ChunkColumn> ChunkColumns;
        std::set<std::pair<int, int>> loadedChunkPositions;
        TextureArray textureArray;
        FastNoise::SmartNode<FastNoise::Simplex> noise;

};