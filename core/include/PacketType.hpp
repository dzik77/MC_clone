#pragma once
#include <cstdint>
#include <WorldConfig.hpp>
#include <Block.hpp>

enum class PacketType : uint8_t {
    Chunk = 1,
    RequestChunk = 2,
    PlayerMove = 3,
    ServerAuthoritativeState = 4
};

#pragma pack(push, 1)

struct ChunkPacket {
    const PacketType type = PacketType::Chunk;
    int32_t x,y,z;
    Block chunkBlocks[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT * WorldConfig::CHUNK_SIZE];
};
struct RequestChunkPacket {
    const PacketType type = PacketType::RequestChunk;
    int32_t chunkColumnX;
    int32_t chunkColumnZ;
};
struct PlayerMovePacket {
    const PacketType type = PacketType::PlayerMove;
    uint32_t sequenceID;
    glm::vec3 position;
};
struct ServerAuthoritativeStatePacket {
    const PacketType type = PacketType::ServerAuthoritativeState;
    uint32_t lastProcessedInputSequenceID;
    glm::vec3 truePosition;
    bool wasRejected;
};

#pragma pack(pop)