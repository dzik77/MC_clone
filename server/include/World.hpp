#pragma once

#include <FastNoise/FastNoise.h>
#include <TerrainSpline.hpp>
#include <WorldConfig.hpp>
#include <glm/glm.hpp>
#include <PacketType.hpp>

class World {
    public:

        World();
        ~World();
        void generateTerrain(glm::ivec2 chunkColumnPosition, ChunkPacket* chunkPackets);

    private:

    // Noise generator for terrain generation
    FastNoise::SmartNode<FastNoise::Simplex> continentalness, erosion, peaksAndValleys;
    FastNoise::SmartNode<FastNoise::FractalFBm> erosionFractal;
    FastNoise::SmartNode<FastNoise::FractalFBm> peaksAndValleysFractal;

    // Terrain spline for height evaluation
    TerrainSpline continentalnessSpline;
    TerrainSpline erosionSpline;
    TerrainSpline peaksAndValleysSpline;

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