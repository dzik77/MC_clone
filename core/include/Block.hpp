#pragma once
#include <cstdint>
#include <Direction.hpp>

enum class Block: uint8_t {
    AIR,
    DIRT,
    GRASS,
    STONE
};

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
    }
    return 0;
}