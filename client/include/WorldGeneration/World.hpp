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
#include <IWorldAccesor.hpp>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <ankerl/unordered_dense.h>

#include <enet/enet.h>
#include <PacketType.hpp>

constexpr int PREALLOCATE_CHUNKS = 150000;

class World : public IWorldAccessor {
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
        
        void startMeshGenerationThreads() {
            meshGenerationThreadsRunning = true;
            for (int i = 0; i < meshGenerationThreadCount; i++) {
                meshGenerationThreads.emplace_back(&World::meshGenerationWorkerThread, this);
            }
        }
        void stopMeshGenerationThreads() {
            meshGenerationThreadsRunning = false;
            chunksToGenerateMeshQueue_cv.notify_all();
            for (auto& thread : meshGenerationThreads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
        }

        // multithreading
        void uploadChunksToGPU();
        
        private:
        
        // networking
        ENetPeer* serverPeer; // MUST BE SET
        
        void loadTextures();
        TextureArray textureArray;
        
        // render distance
        int renderDistance = 32;
        std::vector<glm::ivec2> chunksToLoadOffset; // based on render distance, calculated in setRenderDistance
        
        // allocator
        VramAllocator vramAllocator;
        GLuint globalVAO;
        
        // Chunks data proccesing
        struct IVec2Hash {
            std::size_t operator()(const glm::ivec2& k) const {
                return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1);
            }
        };
        
        ankerl::unordered_dense::map<glm::ivec2, std::shared_ptr<ChunkColumn>, IVec2Hash> chunkColumn;
        std::stack<int> freeChunkPositionIndices;
        std::stack<glm::ivec2> chunksToGenerateMesh;
        ankerl::unordered_dense::set<glm::ivec2, IVec2Hash> chunksGeneratedOnEdge;
        
        // Chunks waiting for mesh generation
        struct MeshJob {
            std::shared_ptr<ChunkColumn> chunkColumn;
            int columnHeight;
            std::shared_ptr<ChunkColumn> chunkColumn_Front;
            std::shared_ptr<ChunkColumn> chunkColumn_Back;
            std::shared_ptr<ChunkColumn> chunkColumn_Left;
            std::shared_ptr<ChunkColumn> chunkColumn_Right;
            int timestamp;
        };
        std::queue<MeshJob> chunksToGenerateMeshQueue;
        std::mutex chunksToGenerateMeshQueue_mutex;
        std::condition_variable chunksToGenerateMeshQueue_cv;
        const int meshesToGeneratePerThread = 10;
        const int meshGenerationThreadCount = 2;
        bool meshGenerationThreadsRunning = true;
        void meshGenerationWorkerThread(); // Worker thread for mesh generation
        std::vector<std::thread> meshGenerationThreads;

        // Chunks waiting for mesh upload to GPU
        std::queue<glm::ivec3> chunksToUploadToGPU;
        std::mutex chunksToUploadToGPU_mutex;

        const int chunksToUploadToGPUPerFrame = 10; // Limit the number of chunks uploaded to GPU per frame to avoid stalling the GPU

        // IWorldAccessor implementation
        Block getBlockAt(const glm::ivec3& position) const override;

        // Shader Storage Buffer Object (SSBO) for chunk positions
        GLuint chunkSSBO;
        ChunkPosition* ssboData;

        // Indirect draw commands buffer
        GLuint indirectCommandBuffer;
        DrawElementsIndirectCommand* mappedIndirectCommands;
        size_t indirectCommandBufferSize;

};