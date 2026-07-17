#pragma once
#include <cstdint>

struct ChunkColumnPacket {
    uint8_t packet_id = 1;
    uint8_t chunkColumnBlocks[16 * (16 * 16) * 16];
};