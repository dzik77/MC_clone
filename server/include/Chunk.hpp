#pragma once
#include <Block.hpp>
#include <WorldConfig.hpp>
#include <cstring>
#include <enet/enet.h>

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
    int playersLoadedChunkColumn = 0; // Number of players that have loaded this chunk column
};
struct ChunkToGenerate {
    int x;
    int z;
    bool send;
    ENetPeer* peer;
};
struct ChunkToSend {
    ChunkColumn chunkPacket;
    bool upload = false;
    ENetPeer* peer;
};