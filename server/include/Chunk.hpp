#pragma once
#include <Block.hpp>
#include <WorldConfig.hpp>
#include <cstring>
#include <enet/enet.h>
#include <memory>

enum class ChunkColumnState : uint8_t {
    UNLOADED ,
    BASE_TERRAIN_GENERATING,
    BASE_TERRAIN_DONE,
    DECORATION_GENERATING,
    READY
};
struct ChunkColumn{
    ChunkColumn() {
        std::memset(blocks, (int)Block::AIR, sizeof(blocks));
    }
    ChunkColumnState state = ChunkColumnState::UNLOADED;
    Block blocks[WorldConfig::CHUNK_SIZE][WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE];
    glm::ivec2 position;
};
struct ChunkToGenerate {
    int x;
    int z;
    ENetPeer* peer;
};
struct ChunkToSend {
    std::shared_ptr<ChunkColumn> chunkPacket;
    bool upload = false;
    ENetPeer* peer;
};