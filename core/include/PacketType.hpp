#pragma once
#include <cstdint>

enum class PacketType : uint8_t {
    Chunk = 1,
    RequestChunk = 2
};

#pragma pack(push, 1)

struct ChunkPacket {
    PacketType type = PacketType::Chunk;
    uint8_t x,y,z;
    uint8_t chunkColumnBlocks[16 * 16 * 16];
};
struct RequestChunkPacket {
    PacketType type = PacketType::RequestChunk;
    int32_t chunkColumnX;
    int32_t chunkColumnZ;
};

#pragma pack(pop)