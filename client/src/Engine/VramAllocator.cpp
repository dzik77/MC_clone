#include <Engine/VramAllocator.hpp>

VramAllocator::VramAllocator(std::size_t MAX_VERTICES, std::size_t MAX_INDICES) {

    VBOSize = MAX_VERTICES * sizeof(uint32_t);
    EBOSize = MAX_INDICES * sizeof(uint16_t);

    glCreateBuffers(1, &VBO);
    glCreateBuffers(1, &EBO);

    glNamedBufferStorage(VBO, VBOSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(EBO, EBOSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

    //vertexData = (uint32_t*)glMapNamedBufferRange(VBO, 0, VBOSize, accessFlags);
    //indexData = (uint16_t*)glMapNamedBufferRange(EBO, 0, EBOSize, accessFlags);

    freeVertexBlocks.insert({0, MAX_VERTICES});
    freeVertexBlocksOffset.insert({0, MAX_VERTICES});
    freeIndexBlocks.insert({0, MAX_INDICES});
    freeIndexBlocksOffset.insert({0, MAX_INDICES});

}

VramAllocator::~VramAllocator(){

    freeVertexBlocks.clear();
    freeVertexBlocksOffset.clear();
    freeIndexBlocks.clear();
    freeIndexBlocksOffset.clear();

    //vertexData = nullptr;
    //indexData = nullptr;
    VBO = EBO = 0;

}

void VramAllocator::initialize(std::size_t MAX_VERTICES, std::size_t MAX_INDICES) {
    
    VBOSize = MAX_VERTICES * sizeof(uint32_t);
    EBOSize = MAX_INDICES * sizeof(uint16_t);

    glCreateBuffers(1, &VBO);
    glCreateBuffers(1, &EBO);

    glNamedBufferStorage(VBO, VBOSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferStorage(EBO, EBOSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

    //vertexData = (uint32_t*)glMapNamedBufferRange(VBO, 0, VBOSize, accessFlags);
    //indexData = (uint16_t*)glMapNamedBufferRange(EBO, 0, EBOSize, accessFlags);

    freeVertexBlocks.insert({0, MAX_VERTICES});
    freeVertexBlocksOffset.insert({0, MAX_VERTICES});
    freeIndexBlocks.insert({0, MAX_INDICES});
    freeIndexBlocksOffset.insert({0, MAX_INDICES});

}

void VramAllocator::clean(){

    glUnmapNamedBuffer(VBO);
    glUnmapNamedBuffer(EBO);

    //vertexData = nullptr;
    //indexData = nullptr;

    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    freeVertexBlocks.clear();
    freeVertexBlocksOffset.clear();
    freeIndexBlocks.clear();
    freeIndexBlocksOffset.clear();

    //vertexData = nullptr;
    //indexData = nullptr;
    VBO = EBO = 0;

}

BufferAllocation VramAllocator::allocate(std::size_t vertexCount, std::size_t indexCount) {
    std::lock_guard<std::mutex> lock(allocatorMutex);
    vertexCount += VERTEX_FAT_ALLOCATION;
    indexCount += INDEX_FAT_ALLOCATION;
    BufferAllocation allocation = {0, 0, vertexCount, indexCount};

    auto freeVertexIt = freeVertexBlocks.lower_bound({0, vertexCount});
    if (freeVertexIt == freeVertexBlocks.end()) {
        throw std::runtime_error("Not enough vertex buffer memory available");
    }
    auto freeVertexOffsetIt = freeVertexBlocksOffset.find({freeVertexIt->offset, freeVertexIt->size});
    if (freeVertexOffsetIt == freeVertexBlocksOffset.end()) {
        throw std::runtime_error("Not enough vertex buffer memory available");
    }
    auto freeIndexIt = freeIndexBlocks.lower_bound({0, indexCount});
    if (freeIndexIt == freeIndexBlocks.end()) {
        throw std::runtime_error("Not enough index buffer memory available");
    }
    auto freeIndexOffsetIt = freeIndexBlocksOffset.find({freeIndexIt->offset, freeIndexIt->size});
    if (freeIndexOffsetIt == freeIndexBlocksOffset.end()) {
        throw std::runtime_error("Not enough index buffer memory available");
    }

    allocation.vertexBlocks = freeVertexIt->offset;
    allocation.indexBlocks = freeIndexIt->offset;

    if (freeVertexIt->size > vertexCount) {
        freeVertexBlocks.insert(VramBlock{freeVertexIt->offset + vertexCount, freeVertexIt->size - vertexCount});
        freeVertexBlocksOffset.insert(VramBlock{freeVertexIt->offset + vertexCount, freeVertexIt->size - vertexCount});
    }
    freeVertexBlocks.erase(freeVertexIt);
    freeVertexBlocksOffset.erase(freeVertexOffsetIt);

    if (freeIndexIt->size > indexCount) {
        freeIndexBlocks.insert(VramBlock{freeIndexIt->offset + indexCount, freeIndexIt->size - indexCount});
        freeIndexBlocksOffset.insert(VramBlock{freeIndexIt->offset + indexCount, freeIndexIt->size - indexCount});
    }
    freeIndexBlocks.erase(freeIndexIt);
    freeIndexBlocksOffset.erase(freeIndexOffsetIt);

    return allocation;
}

void VramAllocator::free(const BufferAllocation &allocation) {
    std::lock_guard<std::mutex> lock(allocatorMutex);

    VramBlock vertexBlock = {allocation.vertexBlocks, allocation.vertexCount};
    auto rightVertexBlockOffsetIt = freeVertexBlocksOffset.upper_bound({vertexBlock.offset, 0});
    if (rightVertexBlockOffsetIt != freeVertexBlocksOffset.end()) {
        auto rightVertexBlockIt = freeVertexBlocks.find({rightVertexBlockOffsetIt->offset, rightVertexBlockOffsetIt->size});
        if (rightVertexBlockIt != freeVertexBlocks.end() && vertexBlock.offset + vertexBlock.size == rightVertexBlockIt->offset) {
            vertexBlock.size += rightVertexBlockIt->size;
            freeVertexBlocksOffset.erase(rightVertexBlockOffsetIt);
            freeVertexBlocks.erase(rightVertexBlockIt);
        }
    }
    auto leftVertexBlockOffsetIt = freeVertexBlocksOffset.lower_bound({vertexBlock.offset, 0});
    if (leftVertexBlockOffsetIt != freeVertexBlocksOffset.begin()){
        leftVertexBlockOffsetIt--;
        auto leftVertexBlockIt = freeVertexBlocks.find({leftVertexBlockOffsetIt->offset, leftVertexBlockOffsetIt->size});
        if (leftVertexBlockIt != freeVertexBlocks.end() && leftVertexBlockOffsetIt->offset + leftVertexBlockOffsetIt->size == vertexBlock.offset) {
            vertexBlock.offset = leftVertexBlockOffsetIt->offset;
            vertexBlock.size += leftVertexBlockOffsetIt->size;
            freeVertexBlocks.erase(leftVertexBlockIt);
            freeVertexBlocksOffset.erase(leftVertexBlockOffsetIt);
        }
    }
    freeVertexBlocks.insert(vertexBlock);
    freeVertexBlocksOffset.insert(vertexBlock);

    VramBlock indexBlock = {allocation.indexBlocks, allocation.indexCount};
    auto rightIndexBlockOffsetIt = freeIndexBlocksOffset.upper_bound({indexBlock.offset, 0});
    if (rightIndexBlockOffsetIt != freeIndexBlocksOffset.end()) {
        auto rightIndexBlockIt = freeIndexBlocks.find({rightIndexBlockOffsetIt->offset, rightIndexBlockOffsetIt->size});
        if (rightIndexBlockIt != freeIndexBlocks.end() && indexBlock.offset + indexBlock.size == rightIndexBlockIt->offset) {
            indexBlock.size += rightIndexBlockIt->size;
            freeIndexBlocksOffset.erase(rightIndexBlockOffsetIt);
            freeIndexBlocks.erase(rightIndexBlockIt);
        }
    }
    auto leftIndexBlockOffsetIt = freeIndexBlocksOffset.lower_bound({indexBlock.offset, 0});
    if (leftIndexBlockOffsetIt != freeIndexBlocksOffset.begin()){
        leftIndexBlockOffsetIt--;
        auto leftIndexBlockIt = freeIndexBlocks.find({leftIndexBlockOffsetIt->offset, leftIndexBlockOffsetIt->size});
        if (leftIndexBlockIt != freeIndexBlocks.end() && leftIndexBlockOffsetIt->offset + leftIndexBlockOffsetIt->size == indexBlock.offset) {
            indexBlock.offset = leftIndexBlockOffsetIt->offset;
            indexBlock.size += leftIndexBlockOffsetIt->size;
            freeIndexBlocks.erase(leftIndexBlockIt);
            freeIndexBlocksOffset.erase(leftIndexBlockOffsetIt);
        }
    }
    freeIndexBlocks.insert(indexBlock);
    freeIndexBlocksOffset.insert(indexBlock);
}

BufferAllocation VramAllocator::reallocate(const BufferAllocation &oldAllocation, std::size_t newVertexCount, std::size_t newIndexCount)
{
    BufferAllocation newAllocation = allocate(newVertexCount, newIndexCount);

    /*
    std::memcpy(
        (void *)(vertexData + newAllocation.vertexBlocks), 
        (void *)(vertexData + oldAllocation.vertexBlocks), 
        std::min(oldAllocation.vertexCount, newAllocation.vertexCount) * sizeof(uint32_t));
        std::memcpy(
            (void *)(indexData + newAllocation.indexBlocks), 
            (void *)(indexData + oldAllocation.indexBlocks), 
            std::min(oldAllocation.indexCount, newAllocation.indexCount) * sizeof(uint16_t));
    */
    
    std::size_t vertexCopyCount = std::min(oldAllocation.vertexCount, newAllocation.vertexCount);
    std::size_t indexCopyCount = std::min(oldAllocation.indexCount, newAllocation.indexCount);
    glCopyNamedBufferSubData(VBO, VBO, oldAllocation.vertexBlocks * sizeof(uint32_t), newAllocation.vertexBlocks * sizeof(uint32_t), vertexCopyCount * sizeof(uint32_t));
    glCopyNamedBufferSubData(EBO, EBO, oldAllocation.indexBlocks * sizeof(uint16_t), newAllocation.indexBlocks * sizeof(uint16_t), indexCopyCount * sizeof(uint16_t));
    
    free(oldAllocation);
    
    return newAllocation;
}
