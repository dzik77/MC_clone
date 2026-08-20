#pragma once

#include <FastNoise/FastNoise.h>
#include <TerrainSpline.hpp>
#include <WorldConfig.hpp>
#include <glm/glm.hpp>
#include <PacketType.hpp>
#include <ankerl/unordered_dense.h>
#include <IWorldAccesor.hpp>
#include <memory>
#include <Chunk.hpp>
#include <queue>
#include <mutex>
#include <hash.hpp>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <ChunkCompression.hpp>
#include <cstring>

// One compressed, ready-to-send chunk packet (header + variable-length compressed body).
struct ChunkSendBuffer {
    uint8_t data[sizeof(ChunkPacketHeader) + ChunkCompression::MAX_BODY_SIZE];
    size_t length;
};

class World : public IWorldAccessor {
    public:

        World();
        ~World();

        // terain generation
        void playerLoadedChunkColumn(glm::ivec2 chunkColumnPosition){
            chunkColumnPlayersLoadedCount[chunkColumnPosition]++;
        }
        int playersLoadedChunkColumn(glm::ivec2 chunkColumnPosition){
            if (chunkColumnPlayersLoadedCount.find(chunkColumnPosition) != chunkColumnPlayersLoadedCount.end()){
                return chunkColumnPlayersLoadedCount[chunkColumnPosition];
            } else {
                return 0;
            }
        }
        void sentAndLoadChunkColumn(glm::ivec2 chunkColumnPosition, ENetPeer* peer);
        void generateTerrain(glm::ivec2 chunkColumnPosition, ENetPeer* peer);
        ChunkSendBuffer* chunkColumnToPacket(ChunkColumn& chunkColumn);
        void uploadChunk(std::shared_ptr<ChunkColumn> chunkColumn){
            chunkColumns[chunkColumn->position] = std::move(chunkColumn);
        }

        void unloadChunkColumns(std::vector<glm::ivec2> &chunkColumnsToUnload);

        void loadedChunkColumn(){
            static int lastSize = 0;
            if (chunkColumns.size() != lastSize){
                lastSize = chunkColumns.size();
                std::cout << "Loaded chunk columns: " << chunkColumns.size() << std::endl;
            }
            static int lastSizePlayers = 0;
            if (chunkColumnPlayersLoadedCount.size() != lastSizePlayers){
                lastSizePlayers = chunkColumnPlayersLoadedCount.size();
                std::cout << "Players loaded chunk columns: " << chunkColumnPlayersLoadedCount.size() << std::endl;
            }
        }
        
        // IWorldAccessor interface
        void updateBlock(glm::ivec3 position, Block block) override;
        
        // Multithreading
        std::condition_variable chunksToGenerateCV;
       
        std::queue<ChunkToSend> chunksToSend;
        std::mutex chunksToSendMutex;
        
        void startTerrainGenerationThreads() {
            terrainGenerationThreadsRunning = true;
            for (int i = 0; i < terrainGenerationThreadsCount; i++) {
                terrainGenerationThreads.emplace_back(&World::terrainGenerationWorkerThread, this);
            }
        }
        void stopTerrainGenerationThreads() {
            terrainGenerationThreadsRunning = false;
            chunksToGenerateCV.notify_all();
            for (auto& thread : terrainGenerationThreads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            terrainGenerationThreads.clear();
        }

    private:
        
    // Noise generator for terrain generation
    FastNoise::SmartNode<FastNoise::Simplex> continentalness, erosion, peaksAndValleys, vegetation;
    FastNoise::SmartNode<FastNoise::FractalFBm> erosionFractal;
    FastNoise::SmartNode<FastNoise::FractalFBm> peaksAndValleysFractal;
    
    // Terrain spline for height evaluation
    TerrainSpline continentalnessSpline;
    TerrainSpline erosionSpline;
    TerrainSpline peaksAndValleysSpline;

    uint32_t squirrel3(uint32_t n, uint32_t seed) {
        constexpr uint32_t BIT_NOISE1 = 0x68E31DA4;
        constexpr uint32_t BIT_NOISE2 = 0xB5297A4D;
        constexpr uint32_t BIT_NOISE3 = 0x1B56C4E9;
        n *= BIT_NOISE1;
        n += seed;
        n ^= (n >> 8);
        n += BIT_NOISE2;
        n ^= (n << 8);
        n *= BIT_NOISE3;
        n ^= (n >> 8);
        return n;
    }
    uint32_t hashCell(int cellX, int cellZ) {
        constexpr uint32_t PRIME = 198491317u;
        return squirrel3(static_cast<uint32_t>(cellX) + PRIME * static_cast<uint32_t>(cellZ), static_cast<uint32_t>(seed));
    }

    // Margin of extra noise sampled past each column's edge, for a future decoration pass to read without needing neighboring columns generated
    static constexpr int borderMargin = 8;
    static constexpr int treeMargin = 5;

    // Chunk data
    ankerl::unordered_dense::map<glm::ivec2, std::shared_ptr<ChunkColumn>, IVec2Hash> chunkColumns; // access only on main thread
    ankerl::unordered_dense::map<glm::ivec2, int, IVec2Hash> chunkColumnPlayersLoadedCount; // access only on main thread

    // Multithreading
    const int chunkToGeneratePerThread = 10;
    const int terrainGenerationThreadsCount = 4;
    bool terrainGenerationThreadsRunning = true;
    void terrainGenerationWorkerThread();
    std::vector<std::thread> terrainGenerationThreads;

    // Multithreading synchronization
    std::queue<ChunkToGenerate> chunksToGenerate;
    std::mutex chunksToGenerateMutex;

    Block getBlockAt(const glm::ivec3& position) const override;

    Block getBlockFromHeight(int y, int height){
        if (y <= height-3){
            return Block::STONE;
        } else if (y <= height-1 || (y == height && height < waterLevel)){
            return Block::DIRT;
        } else if (y == height){
            return Block::GRASS;
        } else if (y > height && y <= waterLevel){
            return Block::WATER;
        } else {
            return Block::AIR;
        }
    }
    const int waterLevel = 62; // Water level in the world

    int seed = 67;

};