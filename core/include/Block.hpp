#pragma once
#include <cstdint>
#include <Direction.hpp>

constexpr uint8_t TRANSPARENCY_THRESHOLD = 255;
enum class Block: uint8_t {
    AIR = 0,
    DIRT = 1,
    GRASS = 2,
    STONE = 3,
    LOG = 4,
    LEAVES = 5,
    WATER = 255
};

inline bool isSolid(Block block){
    return block != Block::AIR && (uint8_t)block < TRANSPARENCY_THRESHOLD;
}
inline bool isTransparent(Block block){
    return (uint8_t)block >= TRANSPARENCY_THRESHOLD;
}

inline int getTextureID(Block block, Direction direction){
    switch (block) {
        case Block::AIR:
            return 0;
        case Block::DIRT:
            return 1;
        case Block::GRASS:
            if (direction == Direction::UP) 
                return 3;
            else if (direction == Direction::DOWN)
                return 1;
            else
                return 2;
        case Block::STONE:
            return 4;
        case Block::WATER:
            return 5;
        case Block::LOG:
            if (direction == Direction::UP || direction == Direction::DOWN)
                return 6;
            else
                return 7;
        case Block::LEAVES:
            return 8;
    }
    return 0;
}