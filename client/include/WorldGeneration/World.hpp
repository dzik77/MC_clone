#pragma once
#include <glm/glm.hpp>
#include <WorldGeneration/Chunk.hpp>
#include <vector>
#include <Engine/Shaders.hpp>
#include <Engine/TextureManager.hpp>
#include <FastNoise/FastNoise.h>
#include <set>
#include <Engine/VramAllocator.hpp>
#include <Engine/Engine.hpp>
#include <stack>
#include <map>
#include <queue>
#include <numeric>

#include <enet/enet.h>
#include <PacketType.hpp>

constexpr int PREALLOCATE_CHUNKS = 150000;

class World{
    public:
    
        World();
        void initialize();
        void generateTerrain(ChunkColumn &chunkColumn);
        void loadTerrain();
        void generateMesh();
        void loadChunks(glm::vec3 playerPosition);
        void render(Shader& shader, glm::vec3 playerPosition);
        void clean();
        void setServerPeer(ENetPeer* peer) { serverPeer = peer; }
        ~World();

        std::queue<ChunkPacket> receivedChunks;

        void setRenderDistance(int distance);
        
        private:
        
        // networking
        ENetPeer* serverPeer; // MUST BE SET

        void loadTextures();
        TextureArray textureArray;
        
        // render distance
        int renderDistance = 32;
        std::vector<std::pair<int,int>> chunksToLoadOffset; // based on render distance, calculated in setRenderDistance

        // allocator
        VramAllocator vramAllocator;
        GLuint globalVAO;
        
        // Chunks data proccesing
        std::vector<ChunkColumn> ChunkColumns;
        std::vector<int> sortedChunkColumnsIndices; // sorted by distance to player, used for mesh generation
        std::stack<int> freeChunkPositionIndices;
        std::stack<int> chunksToGenerateMesh;
        std::set<int> chunksGeneratedOnEdge;
        std::map<std::pair<int, int>, int> loadedChunkPositions;
        std::map<std::pair<int, int>, int> uploadedChunkPositions;

        // Shader Storage Buffer Object (SSBO) for chunk positions
        GLuint chunkSSBO;
        ChunkPosition* ssboData;

        // Indirect draw commands buffer
        GLuint indirectCommandBuffer;
        DrawElementsIndirectCommand* mappedIndirectCommands;
        size_t indirectCommandBufferSize;

};