#pragma once
#include <glm/glm.hpp>
#include <Block.hpp>

class IWorldAccessor {
    public:
        virtual ~IWorldAccessor() = default;

        virtual Block getBlockAt(const glm::ivec3& position) const = 0;
        virtual void updateBlock(glm::ivec3 position, Block block) = 0;
};