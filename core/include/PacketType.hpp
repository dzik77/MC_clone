#pragma once
#include <cstdint>
#include <WorldConfig.hpp>
#include <Block.hpp>

enum class PacketType : uint8_t {
    Chunk = 1,
    RequestChunk = 2
};

#pragma pack(push, 1)

struct ChunkPacket {
    PacketType type = PacketType::Chunk;
    int32_t x,y,z;
    Block chunkBlocks[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT * WorldConfig::CHUNK_SIZE];
};
struct RequestChunkPacket {
    PacketType type = PacketType::RequestChunk;
    int32_t chunkColumnX;
    int32_t chunkColumnZ;
};

#pragma pack(pop)