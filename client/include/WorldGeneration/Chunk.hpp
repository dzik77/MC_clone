#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <WorldConfig.hpp>
#include <Direction.hpp>
#include <Engine/VramAllocator.hpp>
#include <utility>
#include <Block.hpp>
#include <atomic>
#include <memory>
#include <mutex>

struct ChunkPosition{
    glm::vec4 position;
};

class ChunkColumn;

void generateMeshForChunkThreadSafe(
    std::shared_ptr<ChunkColumn> chunk, int columnHeight,
    std::shared_ptr<ChunkColumn> blocks_Front,
    std::shared_ptr<ChunkColumn> blocks_Back,
    std::shared_ptr<ChunkColumn> blocks_Left,
    std::shared_ptr<ChunkColumn> blocks_Right
);

class Chunk{
    public:
    Chunk();
    Chunk(VramAllocator* vramAllocator);
    ~Chunk();

    void init(VramAllocator* vramAllocator);

    // Delete copy semantics to prevent accidental shallow copies
    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    // Declare move semantics
    Chunk(Chunk&& other) noexcept;
    Chunk& operator=(Chunk&& other) noexcept;

    void generateMesh(
        Block (*blocks)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE], int columnHeight,
        Block (*blocks_Front)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE],
        Block (*blocks_Back)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE],
        Block (*blocks_Left)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE],
        Block (*blocks_Right)[WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE]);   

    // <-- Mesh data for GPU (needed for multi-threading) -->
    std::vector<uint32_t> verticesOpaque, verticesTransparent;
    std::vector<uint16_t> indiciesOpaque, indiciesTransparent;
    bool isMeshGenerated = false; // Flag to indicate if the mesh has been generated
    
    bool hasMeshToUpload = false; // Flag to indicate if the mesh has been generated and needs to be uploaded to GPU (needs mutex)
    int timeStamp = 0; // acquire meshMutex
    std::mutex meshMutex; // Mutex to protect access to mesh data

    // Mesh data
    int verticesCountOpaque = 0;
    int indiciesCountOpaque = 0;
    int verticesCountTransparent = 0;
    int indiciesCountTransparent = 0;
    uint32_t ssboIndex = -1;
    VramAllocator* vramAllocator;
    BufferAllocation chunkAllocationOpaque, chunkAllocationTransparent;

    private:
};

Chunk create_chunk(glm::ivec3 chunkPosition);
inline uint32_t create_vertex(int x, int y, int z, uint8_t TextureID, int Direction, int UVCoordinate, int AmbientOcclusion = 0) {
    /*
        x - 5 bits
        y - 5 bits
        z - 5 bits
        TextureID - 8 bits
        Direction - 3 bits
        UVCoordinate - 2bits
        AmbientOcclusion - 2 bits
    */
    return ((x & 0x1F) | ((y & 0x1F) << 5) | ((z & 0x1F) << 10) | ((TextureID & 0xFF) << 15) | ((Direction & 0x7) << 23) | ((UVCoordinate & 0x3) << 26)) | ((AmbientOcclusion & 0x3) << 28);
}
inline int calculateVertexAO(bool side1, bool side2, bool corner) {
    if (side1 && side2) return 3;
    return side1 + side2 + corner;
}



class ChunkColumn{
    public:
    
    ChunkColumn(glm::vec2 position, VramAllocator* vramAllocator);
    ~ChunkColumn();

    ChunkColumn(const ChunkColumn&) = delete;
    ChunkColumn& operator=(const ChunkColumn&) = delete;

    ChunkColumn(ChunkColumn&& other) noexcept;
    ChunkColumn& operator=(ChunkColumn&& other) noexcept;

    Block blocks[WorldConfig::CHUNK_SIZE][WorldConfig::CHUNK_SIZE * WorldConfig::CHUNK_COLUMN_HEIGHT][WorldConfig::CHUNK_SIZE];
    Chunk chunks[WorldConfig::CHUNK_COLUMN_HEIGHT];
    glm::vec2 position;
    VramAllocator* vramAllocator;

    int loadedChunks = 0;

#ifdef DEBUG
    enum class ChunkDebugState {
        Unrequested,
        Requested,
        InMeshingQueue,
        Ready
    };
    std::atomic<ChunkDebugState> debugState{ChunkDebugState::Unrequested};
#endif

    private:
};