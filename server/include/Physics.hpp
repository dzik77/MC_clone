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


class Physics {

    public:

        Physics() : deltaTime(0.0f) {}
        ~Physics() {}

        void update(float dt);
        void updateEntityPosition()

    private:

        float gravity = -9.81f; // m/s^2
        float deltaTime;

};