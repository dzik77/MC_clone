#pragma once
#include <enet/enet.h>
#include <glm/glm.hpp>

struct Player {
    ENetPeer* peer;
    glm::vec3 position;
    int lastProcessedInputSequenceID;
};