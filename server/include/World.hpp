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

class World : public IWorldAccessor {
    public:

        World();
        ~World();

        // terain generation
        void loadChunkColumn(glm::ivec2 chunkColumnPosition, ENetPeer* peer);
        void sentAndLoadChunkColumn(glm::ivec2 chunkColumnPosition, ENetPeer* peer);
        void generateTerrain(glm::ivec2 chunkColumnPosition, ENetPeer* peer);
        ChunkPacket* chunkColumnToPacket(ChunkColumn& chunkColumn);
        void uploadChunkColumn(ChunkColumn chunkColumn){
            std::shared_ptr<ChunkColumn> chunkColumnPtr = std::make_shared<ChunkColumn>(chunkColumn);
            chunkColumns[chunkColumn.position] = chunkColumnPtr;
        }

        void unloadChunkColumns(std::vector<glm::ivec2> &chunkColumnsToUnload);

        void loadedChunkColumn(){
            std::cout << "Loaded chunk columns: " << chunkColumns.size() << std::endl;
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
    FastNoise::SmartNode<FastNoise::Simplex> continentalness, erosion, peaksAndValleys;
    FastNoise::SmartNode<FastNoise::FractalFBm> erosionFractal;
    FastNoise::SmartNode<FastNoise::FractalFBm> peaksAndValleysFractal;
    
    // Terrain spline for height evaluation
    TerrainSpline continentalnessSpline;
    TerrainSpline erosionSpline;
    TerrainSpline peaksAndValleysSpline;
    

    // Chunk data
    ankerl::unordered_dense::map<glm::ivec2, std::shared_ptr<ChunkColumn>, IVec2Hash> chunkColumns; // acces only on main thread

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