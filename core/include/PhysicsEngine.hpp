#pragma once
#include <glm/glm.hpp>
#include <IWorldAccesor.hpp>
#include <Block.hpp>

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
    bool onGround = false;
    AABB boundingBox;
};

glm::vec3 updateEntity(Entity& entity, float deltaTime, const IWorldAccessor& worldAccessor);
bool checkOnGround(const Entity& entity, const IWorldAccessor& worldAccessor);

constexpr float gravity = -32.0f; // m/s^2
void applyGravity(Entity& entity, float deltaTime);

class PhysicsEngine {

    public:

        

    private:

};