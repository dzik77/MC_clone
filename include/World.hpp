#pragma once
#include <glm/glm.hpp>
#include <Chunk.hpp>
#include <vector>
#include <Shaders.hpp>
#include <TextureManager.hpp>
#include <FastNoise/FastNoise.h>
#include <set>
#include <VramAllocator.hpp>
#include <Engine.hpp>
#include <stack>
#include <map>

constexpr int PREALLOCATE_CHUNKS = 20000;

class World{
    public:
    
        World();
        void initialize();
        void generateTerrain(ChunkColumn &chunkColumn);
        void generateMesh();
        void loadChunks(glm::vec3 playerPosition);
        void render(Shader& shader);
        void clean();
        ~World();
        
        private:
        
        void loadTextures();
        TextureArray textureArray;
        
        int renderDistance = 32;
        VramAllocator vramAllocator;
        GLuint globalVAO;
        std::vector<ChunkColumn> ChunkColumns;
        std::stack<int> freeChunkPositionIndices;
        std::stack<int> chunksToGenerateMesh;
        std::map<std::pair<int, int>, int> loadedChunkPositions;
        GLuint chunkSSBO;
        ChunkPosition* ssboData;
        GLuint indirectCommandBuffer;
        DrawElementsIndirectCommand* mappedIndirectCommands;
        size_t indirectCommandBufferSize;

        FastNoise::SmartNode<FastNoise::Simplex> noise;

};