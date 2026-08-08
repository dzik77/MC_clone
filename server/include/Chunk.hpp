#pragma once
#include <Block.hpp>
#include <WorldConfig.hpp>
#include <cstring>

struct ChunkColumn{
    ChunkColumn() {
        std::memset(blocks, (int)Block::AIR, sizeof(blocks));
    }
    Block blocks[WorldConfig::CHUNK_SIZE][WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE];
};