#include <Engine/Camera.hpp>

void Camera::KeyboardInput(Direction direction, float deltaTime) {
    float Velocity = MovementSpeed * deltaTime;
    switch(direction){
        case Direction::FORWARD:
            Position += Front * Velocity;
            break;
        case Direction::BACKWARD:
            Position -= Front * Velocity;
            break;
        case Direction::RIGHT:
            Position += Right * Velocity;
            break;
        case Direction::LEFT:
            Position -= Right * Velocity;
            break;
        case Direction::UP:
            Position += WorldUp * Velocity;
            break;
        case Direction::DOWN:
            Position -= WorldUp * Velocity;
            break;
        default:
            break;
    }
}

void Camera::MouseInput(float xoffset, float yoffset) {
    xoffset *= MouseSensivity;
    yoffset *= MouseSensivity;

    Pitch += yoffset;
    Yaw += xoffset;

    if (Pitch > 89.0f)
        Pitch = 89.0f;
    if (Pitch < -89.0f)
        Pitch = -89.0f;

    updateVectors();
}

void Camera::updateVectors() {
    Front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)); 
    Front.y = sin(glm::radians(Pitch));
    Front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch)); 
    Front = glm::normalize(Front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
