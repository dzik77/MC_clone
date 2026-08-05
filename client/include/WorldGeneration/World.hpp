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
        void render(Shader& shader);
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
        
        int renderDistance = 32;
        std::vector<std::pair<int,int>> chunksToLoadOffset;

        VramAllocator vramAllocator;
        GLuint globalVAO;
        std::vector<ChunkColumn> ChunkColumns;
        std::stack<int> freeChunkPositionIndices;
        std::stack<int> chunksToGenerateMesh;
        std::set<int> chunksGeneratedOnEdge;
        std::map<std::pair<int, int>, int> loadedChunkPositions;
        std::map<std::pair<int, int>, int> uploadedChunkPositions;
        GLuint chunkSSBO;
        ChunkPosition* ssboData;
        GLuint indirectCommandBuffer;
        DrawElementsIndirectCommand* mappedIndirectCommands;
        size_t indirectCommandBufferSize;

};