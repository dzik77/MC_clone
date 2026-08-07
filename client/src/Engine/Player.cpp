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

}

void Player::MouseInput(float xoffset, float yoffset) {

    xoffset *= MouseSensivity;
    yoffset *= MouseSensivity;

    Pitch += yoffset;
    Yaw += xoffset;

    if (Pitch > 89.0f)
        Pitch = 89.0f;
    if (Pitch < -89.0f)
        Pitch = -89.0f;

    camera.updateAngles(Yaw, Pitch);

    Front.x = cos(glm::radians(Yaw));
    Front.z = sin(glm::radians(Yaw));
    Right = glm::normalize(glm::cross(Front, WorldUp));

}

void Player::updatePhysics(float deltaTime, const IWorldAccessor &worldAccessor) {

    applyGravity(*this, deltaTime);
    glm::vec3 movementDelta = updateEntity(*this, deltaTime, worldAccessor);
    onGround = checkOnGround(*this, worldAccessor);
    camera.updatePosition(movementDelta);

}
