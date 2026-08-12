#pragma once
#include <glm/glm.hpp>

struct IVec2Hash {
    std::size_t operator()(const glm::ivec2& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1);
    }
};