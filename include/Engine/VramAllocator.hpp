#pragma once

#include <cstddef>
#include <cstdint>
#include <glad/glad.h>
#include <set>
#include <stdexcept>
#include <cstring>

struct Block {
    std::size_t offset;
    std::size_t size;
};
struct CompareBlockByOffset {
    bool operator()(const Block& a, const Block& b) const {
        return a.offset < b.offset;
    }
};
struct CompareBlockBySize {
    bool operator()(const Block& a, const Block& b) const {
        return (a.size < b.size) || (a.size == b.size && a.offset < b.offset);
    }
};

struct BufferAllocation {
    std::size_t vertexBlocks;
    std::size_t indexBlocks;
    std::size_t vertexCount;
    std::size_t indexCount;
};

constexpr uint32_t BLOCKS_FAT_ALLOCATION = 45;
constexpr uint32_t VERTEX_FAT_ALLOCATION = BLOCKS_FAT_ALLOCATION * 6 * 4;
constexpr uint32_t INDEX_FAT_ALLOCATION = BLOCKS_FAT_ALLOCATION * 6 * 4;

class VramAllocator{
    public:
        VramAllocator() {}
        VramAllocator(uint32_t MAX_VERTICES, uint32_t MAX_INDICES);
        ~VramAllocator();

        void initialize(uint32_t MAX_VERTICES, uint32_t MAX_INDICES);
        void clean();

        BufferAllocation allocate(uint32_t vertexCount, uint32_t indexCount);
        void free(const BufferAllocation& allocation);

        BufferAllocation reallocate(const BufferAllocation& oldAllocation, uint32_t newVertexCount, uint32_t newIndexCount);

        GLuint getVertexBufferID() const { return VBO; }
        GLuint getIndexBufferID() const { return EBO; }
        //uint32_t* getVertexData() const { return vertexData; }
        //uint16_t* getIndexData() const { return indexData; }

    private:

        //uint32_t* vertexData;
        //uint16_t* indexData;
        uint32_t VBOSize, EBOSize;
        GLuint VBO, EBO;
        //GLbitfield accessFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        std::set<Block, CompareBlockBySize> freeVertexBlocks, freeIndexBlocks;
        std::set<Block, CompareBlockByOffset> freeVertexBlocksOffset, freeIndexBlocksOffset;
    
};