#pragma once
#include <enet/enet.h>
#include <glm/glm.hpp>
#include <ankerl/unordered_dense.h>
#include <hash.hpp>

struct Player {
    ENetPeer* peer;
    glm::vec3 position;
    glm::ivec2 previousChunkColumnPosition;
    ankerl::unordered_dense::set<glm::ivec2, IVec2Hash> sentChunkColumns;
    int lastProcessedInputSequenceID;
};