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

class World : public IWorldAccessor {
    public:

        World();
        ~World();
        void getTerrain(glm::ivec2 chunkColumnPosition, ChunkPacket* chunkColumnPacket);
        
    private:
        
    // Noise generator for terrain generation
    FastNoise::SmartNode<FastNoise::Simplex> continentalness, erosion, peaksAndValleys;
    FastNoise::SmartNode<FastNoise::FractalFBm> erosionFractal;
    FastNoise::SmartNode<FastNoise::FractalFBm> peaksAndValleysFractal;
    
    // Terrain spline for height evaluation
    TerrainSpline continentalnessSpline;
    TerrainSpline erosionSpline;
    TerrainSpline peaksAndValleysSpline;
    
    // terain generation
    void generateTerrain(glm::ivec2 chunkColumnPosition);

    // Chunk data
    struct IVec2Hash {
        std::size_t operator()(const glm::ivec2& k) const {
            return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1);
        }
    };
    ankerl::unordered_dense::map<glm::ivec2, std::unique_ptr<ChunkColumn>, IVec2Hash> chunkColumns;
    std::mutex chunkColumnsMutex; // Mutex for thread-safe access to chunkColumns

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