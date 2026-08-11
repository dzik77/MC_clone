#include <Engine/Player.hpp>

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
