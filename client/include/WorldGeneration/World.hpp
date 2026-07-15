#pragma once
#include <glm/glm.hpp>
#include <WorldGeneration/Chunk.hpp>
#include <WorldGeneration/TerrainSpline.hpp>
#include <vector>
#include <Engine/Shaders.hpp>
#include <Engine/TextureManager.hpp>
#include <FastNoise/FastNoise.h>
#include <set>
#include <Engine/VramAllocator.hpp>
#include <Engine/Engine.hpp>
#include <stack>
#include <map>

constexpr int PREALLOCATE_CHUNKS = 150000;

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
        int seed = 67;
        VramAllocator vramAllocator;
        GLuint globalVAO;
        std::vector<ChunkColumn> ChunkColumns;
        std::stack<int> freeChunkPositionIndices;
        std::stack<int> chunksToGenerateMesh;
        std::stack<int> chunksGeneratedOnEdge;
        std::map<std::pair<int, int>, int> loadedChunkPositions;
        GLuint chunkSSBO;
        ChunkPosition* ssboData;
        GLuint indirectCommandBuffer;
        DrawElementsIndirectCommand* mappedIndirectCommands;
        size_t indirectCommandBufferSize;

        // Noise generator for terrain generation
        FastNoise::SmartNode<FastNoise::Simplex> continentalness, erosion, peaksAndValleys;
        FastNoise::SmartNode<FastNoise::FractalFBm> erosionFractal;
        FastNoise::SmartNode<FastNoise::FractalFBm> peaksAndValleysFractal;

        // Terrain spline for height evaluation
        TerrainSpline continentalnessSpline;
        TerrainSpline erosionSpline;
        TerrainSpline peaksAndValleysSpline;

};