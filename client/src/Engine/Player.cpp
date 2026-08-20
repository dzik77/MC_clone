#include <Engine/Player.hpp>

void Player::Mesh::create() {
    
    glCreateVertexArrays(1, &VAO);
    glCreateBuffers(1, &VBO);
    glCreateBuffers(1, &EBO);

    glNamedBufferStorage(VBO, vertices.size() * sizeof(Vertex), vertices.data(), 0);
    glNamedBufferStorage(EBO, indices.size() * sizeof(uint16_t), indices.data(), 0);

    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(VAO, EBO);

    glEnableVertexArrayAttrib(VAO, 0);
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribBinding(VAO, 0, 0);

    glEnableVertexArrayAttrib(VAO, 1);
    glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoords));
    glVertexArrayAttribBinding(VAO, 1, 0);

    glEnableVertexArrayAttrib(VAO, 2);
    glVertexArrayAttribIFormat(VAO, 2, 1, GL_INT,  offsetof(Vertex, packed));
    glVertexArrayAttribBinding(VAO, 2, 0);
}

void Player::Mesh::render() {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_SHORT, nullptr);
}

void Player::Mesh::destroy() {


    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    VAO = VBO = EBO = 0;

}

void Player::Mesh::addBox(BodyPart part, glm::vec3 size, glm::vec3 origin,
                           glm::vec2 uvOffset, float inflate)
{
    const float w = size.x, h = size.y, d = size.z;
    const float u = uvOffset.x, v = uvOffset.y;

    const glm::vec3 mn = (origin - glm::vec3(inflate)) / PX_PER_M;
    const glm::vec3 mx = (origin + size + glm::vec3(inflate)) / PX_PER_M;

    // Face order must match FACE_NORMALS in the vertex shader:
    // 0:+X  1:-X  2:+Y  3:-Y  4:+Z  5:-Z
    // Corners are CCW seen from outside: bottom-left, bottom-right, top-right, top-left
    const glm::vec3 corners[6][4] = {
        {{mx.x,mn.y,mx.z},{mx.x,mn.y,mn.z},{mx.x,mx.y,mn.z},{mx.x,mx.y,mx.z}},
        {{mn.x,mn.y,mn.z},{mn.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mn.x,mx.y,mn.z}},
        {{mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z},{mx.x,mx.y,mn.z},{mn.x,mx.y,mn.z}},
        {{mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mx.x,mn.y,mx.z},{mn.x,mn.y,mx.z}},
        {{mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mx.x,mx.y,mx.z},{mn.x,mx.y,mx.z}},
        {{mx.x,mn.y,mn.z},{mn.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z}},
    };

    // Atlas rect per face: {x, y, width, height} in pixels, y measured from the top
    const glm::vec4 rects[6] = {
        { u,             v + d, d, h },
        { u + d + w,     v + d, d, h },
        { u + d,         v,     w, d },
        { u + d + w,     v,     w, d },
        { u + d,         v + d, w, h },
        { u + d + w + d, v + d, w, h },
    };

    for (int f = 0; f < 6; ++f) {
        const float u0 =  rects[f].x                / TEX_SIZE;
        const float u1 = (rects[f].x + rects[f].z)  / TEX_SIZE;
        const float v0 =  rects[f].y                / TEX_SIZE;
        const float v1 = (rects[f].y + rects[f].w)  / TEX_SIZE;

        const glm::vec2 uvs[4] = {{u0,v1},{u1,v1},{u1,v0},{u0,v0}};

        const uint16_t base = static_cast<uint16_t>(vertices.size());
        const int32_t  packed = (static_cast<int32_t>(part) << 3) | f;

        for (int i = 0; i < 4; ++i)
            vertices.push_back({ corners[f][i], uvs[i], packed });

        indices.insert(indices.end(), {
            base, static_cast<uint16_t>(base + 1), static_cast<uint16_t>(base + 2),
            base, static_cast<uint16_t>(base + 2), static_cast<uint16_t>(base + 3)
        });
    }
}

Player::Mesh Player::Mesh::buildHumanoid(bool withOverlay)
{
    Mesh mesh;
    mesh.vertices.reserve(withOverlay ? 288 : 144);
    mesh.indices.reserve(withOverlay ? 432 : 216);

    //           part                 size        origin (rel. to feet, y-up)  uv offset
    mesh.addBox(BodyPart::RightLeg, {4,12,4}, {-4,  0,-2}, { 0, 16});
    mesh.addBox(BodyPart::LeftLeg,  {4,12,4}, { 0,  0,-2}, {16, 48});
    mesh.addBox(BodyPart::Torso,    {8,12,4}, {-4, 12,-2}, {16, 16});
    mesh.addBox(BodyPart::RightArm, {4,12,4}, {-8, 12,-2}, {40, 16});
    mesh.addBox(BodyPart::LeftArm,  {4,12,4}, { 4, 12,-2}, {32, 48});
    mesh.addBox(BodyPart::Head,     {8, 8,8}, {-4, 24,-4}, { 0,  0});

    if (withOverlay) {
        mesh.addBox(BodyPart::RightLeg, {4,12,4}, {-4,  0,-2}, { 0, 32}, 0.25f);
        mesh.addBox(BodyPart::LeftLeg,  {4,12,4}, { 0,  0,-2}, { 0, 48}, 0.25f);
        mesh.addBox(BodyPart::Torso,    {8,12,4}, {-4, 12,-2}, {16, 32}, 0.25f);
        mesh.addBox(BodyPart::RightArm, {4,12,4}, {-8, 12,-2}, {40, 32}, 0.25f);
        mesh.addBox(BodyPart::LeftArm,  {4,12,4}, { 4, 12,-2}, {48, 48}, 0.25f);
        mesh.addBox(BodyPart::Head,     {8, 8,8}, {-4, 24,-4}, {32,  0}, 0.5f);
    }

    return mesh;
}

void Player::handleInput(uint8_t input) {

    glm::vec3 movement(0.0f);
    if (input & 0b1) { // W
        movement += Front;
    }
    if (input & 0b10) { // S
        movement -= Front;
    }
    if (input & 0b100) { // A
        movement -= Right;
    }
    if (input & 0b1000) { // D
        movement += Right;
    }
    if (glm::length(movement) > 0.0f) {
        movement = glm::normalize(movement) * MovementSpeed;
    }
    if ((input & 0b10000) && onGround) { // SPACE
        velocity.y = JumpSpeed;
    }
    velocity = glm::vec3(movement.x, velocity.y, movement.z);

    // Block updates
    if (input & 0b10000000) { // LEFT MOUSE BUTTON
        RaycastResult raycastResult = raycast(camera.getPosition(), camera.getFront(), 5.0f, *worldAccessor);
        if (raycastResult.hit) {
            worldAccessor->updateBlock(raycastResult.hitVoxel, Block::AIR);
            blockUpdate.isUpdate = true;
            blockUpdate.position = raycastResult.hitVoxel;
            blockUpdate.block = Block::AIR;
        } else {
            blockUpdate.isUpdate = false;
        }
    }
    if (input & 0b1000000) { // RIGHT MOUSE BUTTON
        RaycastResult raycastResult = raycast(camera.getPosition(), camera.getFront(), 5.0f, *worldAccessor);
        AABB aabb;
        aabb.min = raycastResult.placementVoxel;
        aabb.max = raycastResult.placementVoxel + glm::vec3(1.0f);
        if (raycastResult.hit && !aabb.intersects(boundingBox)) {
            worldAccessor->updateBlock(raycastResult.placementVoxel, Block::STONE);
            blockUpdate.isUpdate = true;
            blockUpdate.position = raycastResult.placementVoxel;
            blockUpdate.block = Block::STONE;
        } else {
            blockUpdate.isUpdate = false;
        }
    }

    currentState.input = input;

}

void Player::MouseInput(float xoffset, float yoffset) {

    xoffset *= MouseSensivity;
    yoffset *= MouseSensivity;

    Pitch += yoffset;
    Yaw += xoffset;

    currentState.xoffset = xoffset;
    currentState.yoffset = yoffset;

    if (Pitch > 89.0f)
        Pitch = 89.0f;
    if (Pitch < -89.0f)
        Pitch = -89.0f;

    camera.updateAngles(Yaw, Pitch);

    Front.x = cos(glm::radians(Yaw));
    Front.z = sin(glm::radians(Yaw));
    Right = glm::normalize(glm::cross(Front, WorldUp));

}

void Player::updatePhysics(float deltaTime) {

    applyGravity(*this, deltaTime);
    glm::vec3 movementDelta = updateEntity(*this, deltaTime, *worldAccessor);
    onGround = checkOnGround(*this, *worldAccessor);
    camera.updatePosition(movementDelta);
    currentState.predictedPosition = position;
    currentState.yaw = Yaw;
    currentState.pitch = Pitch;
    currentState.dt = deltaTime;

}
