#pragma once
#include <glm/glm.hpp>

struct AABB{
    glm::vec3 min, max;
    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

struct Entity {
    glm::vec3 position;
    glm::vec3 velocity;
    AABB boundingBox;
};

class PhysicsEngine {

    public:

        

    private:

};