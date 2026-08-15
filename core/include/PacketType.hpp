#pragma once
#include <cstdint>
#include <WorldConfig.hpp>
#include <Block.hpp>
#include <glm/glm.hpp>

enum class PacketType : uint8_t {
    Chunk = 1,
    RequestChunk = 2,
    PlayerMove = 3,
    ServerAuthoritativeState = 4,
    PlaceBlock = 5,
    DestroyBlock = 6,
    BlockUpdate = 7,
    PlayerInitialPosition = 8
};

#pragma pack(push, 1)

struct ChunkPacket {
    const PacketType type = PacketType::Chunk;
    int32_t x,y,z;
    Block chunkBlocks[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT * WorldConfig::CHUNK_SIZE];
};
struct RequestChunkPacket { // deprecated, not used anymore, but kept for reference
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
struct PlaceBlockPacket {
    const PacketType type = PacketType::PlaceBlock;
    glm::ivec3 position;
    Block block;
};
struct DestroyBlockPacket {
    const PacketType type = PacketType::DestroyBlock;
    glm::ivec3 position;
};
struct BlockUpdatePacket {
    const PacketType type = PacketType::BlockUpdate;
    glm::ivec3 position;
    Block block;
};
struct PlayerInitialPositionPacket {
    const PacketType type = PacketType::PlayerInitialPosition;
    glm::vec3 initialPosition;
};

#pragma pack(pop)