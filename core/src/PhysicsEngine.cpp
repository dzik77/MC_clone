#include <PhysicsEngine.hpp>
#include <cmath>
#include <algorithm>

void updateEntity(Entity& entity, float deltaTime, const IWorldAccessor& worldAccessor) {
    if (deltaTime <= 0.0f) return;

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
    }
}