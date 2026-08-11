#include <PhysicsEngine.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>

glm::vec3 updateEntity(Entity& entity, float deltaTime, const IWorldAccessor& worldAccessor) {
    glm::vec3 movementDelta(0.0f);
    if (deltaTime <= 0.0f) return movementDelta;

    // -------------------------------------------------------------
    // 1. Y Axis Collision
    // -------------------------------------------------------------
    float dy = entity.velocity.y * deltaTime;
    if (dy != 0.0f) {
        AABB expandedBox = entity.boundingBox;
        if (dy > 0.0f) {
            expandedBox.max.y += dy;
        } else {
            expandedBox.min.y += dy;
        }

        int minX = static_cast<int>(std::floor(expandedBox.min.x));
        int maxX = static_cast<int>(std::floor(expandedBox.max.x));
        int minY = static_cast<int>(std::floor(expandedBox.min.y));
        int maxY = static_cast<int>(std::floor(expandedBox.max.y));
        int minZ = static_cast<int>(std::floor(expandedBox.min.z));
        int maxZ = static_cast<int>(std::floor(expandedBox.max.z));

        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                for (int z = minZ; z <= maxZ; ++z) {
                    Block block = worldAccessor.getBlockAt(glm::ivec3(x, y, z));
                    if (isSolid(block)) {
                        AABB blockAABB;
                        blockAABB.min = glm::vec3(x, y, z);
                        blockAABB.max = glm::vec3(x + 1.0f, y + 1.0f, z + 1.0f);

                        bool overlapX = (entity.boundingBox.min.x < blockAABB.max.x && entity.boundingBox.max.x > blockAABB.min.x);
                        bool overlapZ = (entity.boundingBox.min.z < blockAABB.max.z && entity.boundingBox.max.z > blockAABB.min.z);

                        if (!overlapX || !overlapZ) {
                            continue;
                        }

                        if (dy > 0.0f) {
                            if (entity.boundingBox.min.y < blockAABB.max.y && entity.boundingBox.max.y + dy > blockAABB.min.y) {
                                float maxAllowedDy = blockAABB.min.y - entity.boundingBox.max.y;
                                if (maxAllowedDy < dy) {
                                    dy = std::max(0.0f, maxAllowedDy);
                                    entity.velocity.y = 0.0f;
                                }
                            }
                        } else if (dy < 0.0f) {
                            if (entity.boundingBox.max.y > blockAABB.min.y && entity.boundingBox.min.y + dy < blockAABB.max.y) {
                                float minAllowedDy = blockAABB.max.y - entity.boundingBox.min.y;
                                if (minAllowedDy > dy) {
                                    dy = std::min(0.0f, minAllowedDy);
                                    entity.velocity.y = 0.0f;
                                }
                            }
                        }
                    }
                }
            }
        }

        entity.boundingBox.min.y += dy;
        entity.boundingBox.max.y += dy;
        entity.position.y += dy;
        movementDelta.y = dy;
    }

    // -------------------------------------------------------------
    // 2. X Axis Collision
    // -------------------------------------------------------------
    float dx = entity.velocity.x * deltaTime;
    if (dx != 0.0f) {
        AABB expandedBox = entity.boundingBox;
        if (dx > 0.0f) {
            expandedBox.max.x += dx;
        } else {
            expandedBox.min.x += dx;
        }

        int minX = static_cast<int>(std::floor(expandedBox.min.x));
        int maxX = static_cast<int>(std::floor(expandedBox.max.x));
        int minY = static_cast<int>(std::floor(expandedBox.min.y));
        int maxY = static_cast<int>(std::floor(expandedBox.max.y));
        int minZ = static_cast<int>(std::floor(expandedBox.min.z));
        int maxZ = static_cast<int>(std::floor(expandedBox.max.z));

        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                for (int z = minZ; z <= maxZ; ++z) {
                    Block block = worldAccessor.getBlockAt(glm::ivec3(x, y, z));
                    if (isSolid(block)) {
                        AABB blockAABB;
                        blockAABB.min = glm::vec3(x, y, z);
                        blockAABB.max = glm::vec3(x + 1.0f, y + 1.0f, z + 1.0f);

                        bool overlapY = (entity.boundingBox.min.y < blockAABB.max.y && entity.boundingBox.max.y > blockAABB.min.y);
                        bool overlapZ = (entity.boundingBox.min.z < blockAABB.max.z && entity.boundingBox.max.z > blockAABB.min.z);

                        if (!overlapY || !overlapZ) {
                            continue;
                        }

                        if (dx > 0.0f) {
                            if (entity.boundingBox.min.x < blockAABB.max.x && entity.boundingBox.max.x + dx > blockAABB.min.x) {
                                float maxAllowedDx = blockAABB.min.x - entity.boundingBox.max.x;
                                if (maxAllowedDx < dx) {
                                    dx = std::max(0.0f, maxAllowedDx);
                                    entity.velocity.x = 0.0f;
                                }
                            }
                        } else if (dx < 0.0f) {
                            if (entity.boundingBox.max.x > blockAABB.min.x && entity.boundingBox.min.x + dx < blockAABB.max.x) {
                                float minAllowedDx = blockAABB.max.x - entity.boundingBox.min.x;
                                if (minAllowedDx > dx) {
                                    dx = std::min(0.0f, minAllowedDx);
                                    entity.velocity.x = 0.0f;
                                }
                            }
                        }
                    }
                }
            }
        }

        entity.boundingBox.min.x += dx;
        entity.boundingBox.max.x += dx;
        entity.position.x += dx;
        movementDelta.x = dx;
    }

    // -------------------------------------------------------------
    // 3. Z Axis Collision
    // -------------------------------------------------------------
    float dz = entity.velocity.z * deltaTime;
    if (dz != 0.0f) {
        AABB expandedBox = entity.boundingBox;
        if (dz > 0.0f) {
            expandedBox.max.z += dz;
        } else {
            expandedBox.min.z += dz;
        }

        int minX = static_cast<int>(std::floor(expandedBox.min.x));
        int maxX = static_cast<int>(std::floor(expandedBox.max.x));
        int minY = static_cast<int>(std::floor(expandedBox.min.y));
        int maxY = static_cast<int>(std::floor(expandedBox.max.y));
        int minZ = static_cast<int>(std::floor(expandedBox.min.z));
        int maxZ = static_cast<int>(std::floor(expandedBox.max.z));

        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                for (int z = minZ; z <= maxZ; ++z) {
                    Block block = worldAccessor.getBlockAt(glm::ivec3(x, y, z));
                    if (isSolid(block)) {
                        AABB blockAABB;
                        blockAABB.min = glm::vec3(x, y, z);
                        blockAABB.max = glm::vec3(x + 1.0f, y + 1.0f, z + 1.0f);

                        bool overlapX = (entity.boundingBox.min.x < blockAABB.max.x && entity.boundingBox.max.x > blockAABB.min.x);
                        bool overlapY = (entity.boundingBox.min.y < blockAABB.max.y && entity.boundingBox.max.y > blockAABB.min.y);

                        if (!overlapX || !overlapY) {
                            continue;
                        }

                        if (dz > 0.0f) {
                            if (entity.boundingBox.min.z < blockAABB.max.z && entity.boundingBox.max.z + dz > blockAABB.min.z) {
                                float maxAllowedDz = blockAABB.min.z - entity.boundingBox.max.z;
                                if (maxAllowedDz < dz) {
                                    dz = std::max(0.0f, maxAllowedDz);
                                    entity.velocity.z = 0.0f;
                                }
                            }
                        } else if (dz < 0.0f) {
                            if (entity.boundingBox.max.z > blockAABB.min.z && entity.boundingBox.min.z + dz < blockAABB.max.z) {
                                float minAllowedDz = blockAABB.max.z - entity.boundingBox.min.z;
                                if (minAllowedDz > dz) {
                                    dz = std::min(0.0f, minAllowedDz);
                                    entity.velocity.z = 0.0f;
                                }
                            }
                        }
                    }
                }
            }
        }

        entity.boundingBox.min.z += dz;
        entity.boundingBox.max.z += dz;
        entity.position.z += dz;
        movementDelta.z = dz;
    }
    
    return movementDelta;
}

bool checkOnGround(const Entity &entity, const IWorldAccessor &worldAccessor) {
    
    AABB feetBox = entity.boundingBox;
    feetBox.min.y -= 1e-3f;

    int minX = static_cast<int>(std::floor(feetBox.min.x));
    int maxX = static_cast<int>(std::floor(feetBox.max.x));
    int minY = static_cast<int>(std::floor(feetBox.min.y));
    int maxY = static_cast<int>(std::floor(feetBox.max.y));
    int minZ = static_cast<int>(std::floor(feetBox.min.z));
    int maxZ = static_cast<int>(std::floor(feetBox.max.z));

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                Block block = worldAccessor.getBlockAt(glm::ivec3(x, y, z));
                if (isSolid(block)) {
                    AABB blockAABB;
                    blockAABB.min = glm::vec3(x, y, z);
                    blockAABB.max = glm::vec3(x + 1.0f, y + 1.0f, z + 1.0f);

                    bool overlapX = (entity.boundingBox.min.x < blockAABB.max.x - 1e-4f && entity.boundingBox.max.x > blockAABB.min.x + 1e-4f);
                    bool overlapZ = (entity.boundingBox.min.z < blockAABB.max.z - 1e-4f && entity.boundingBox.max.z > blockAABB.min.z + 1e-4f);

                    if (overlapX && overlapZ && feetBox.min.y < blockAABB.max.y && feetBox.max.y >= blockAABB.max.y - 1e-4f) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}


RaycastResult raycast(glm::vec3 origin, glm::vec3 direction, float maxDistance, const IWorldAccessor& worldAccessor) {

    RaycastResult result;
    result.hit = false;

    glm::vec3 currentPosition = origin;
    glm::vec3 previousPosition = currentPosition;

    glm::vec3 step, tDelta, tMax;

    step.x = (direction.x > 0.0f) ? 1.0f : (direction.x < 0.0f) ? -1.0f : 0.0f;
    step.y = (direction.y > 0.0f) ? 1.0f : (direction.y < 0.0f) ? -1.0f : 0.0f;
    step.z = (direction.z > 0.0f) ? 1.0f : (direction.z < 0.0f) ? -1.0f : 0.0f;

    tDelta.x = (direction.x != 0.0f) ? std::abs(1.0f / direction.x) : std::numeric_limits<float>::max();
    tDelta.y = (direction.y != 0.0f) ? std::abs(1.0f / direction.y) : std::numeric_limits<float>::max();
    tDelta.z = (direction.z != 0.0f) ? std::abs(1.0f / direction.z) : std::numeric_limits<float>::max();

    tMax.x = (direction.x > 0.0f) ? (std::floor(currentPosition.x) + 1.0f - currentPosition.x) * tDelta.x :  (currentPosition.x - std::floor(currentPosition.x)) * tDelta.x; // no overflow max 1.0f * maxfloat
    tMax.y = (direction.y > 0.0f) ? (std::floor(currentPosition.y) + 1.0f - currentPosition.y) * tDelta.y : (currentPosition.y - std::floor(currentPosition.y)) * tDelta.y;
    tMax.z = (direction.z > 0.0f) ? (std::floor(currentPosition.z) + 1.0f - currentPosition.z) * tDelta.z : (currentPosition.z - std::floor(currentPosition.z)) * tDelta.z;

    float currentDistance = 0.0f;

    while (currentDistance <= maxDistance) {

        glm::ivec3 blockPos = glm::floor(currentPosition);
        Block block = worldAccessor.getBlockAt(blockPos);
        if (isSolid(block)) {
            result.hit = true;
            result.hitVoxel = glm::vec3(blockPos);
            result.placementVoxel = glm::vec3(glm::floor(previousPosition));
            return result;
        }

        previousPosition = currentPosition;

        if (tMax.x < tMax.y){
            if (tMax.x < tMax.z){
                currentPosition.x += step.x;
                currentDistance = tMax.x;
                tMax.x += tDelta.x;
            } else {
                currentPosition.z += step.z;
                currentDistance = tMax.z;
                tMax.z += tDelta.z;
            }
        } else {
            if (tMax.y < tMax.z){
                currentPosition.y += step.y;
                currentDistance = tMax.y;
                tMax.y += tDelta.y;
            } else {
                currentPosition.z += step.z;
                currentDistance = tMax.z;
                tMax.z += tDelta.z;
            }
        }

    }

    return result;
}

void applyGravity(Entity &entity, float deltaTime) {
    if (!entity.onGround){
        entity.velocity.y += gravity * deltaTime;

        if (entity.velocity.y < -78.4f) {
            entity.velocity.y = -78.4f;
        }

    }
}
