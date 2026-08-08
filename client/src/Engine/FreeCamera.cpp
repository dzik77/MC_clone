#include <Engine/FreeCamera.hpp>

void FreeCamera::KeyboardInput(uint8_t input, float deltaTime) {
    float Velocity = MovementSpeed * deltaTime;
    switch(input){
        case 0b1: // W
            Position += Front * Velocity;
            break;
        case 0b10: // S
            Position -= Front * Velocity;
            break;
        case 0b100: // A
            Position -= Right * Velocity;
            break;
        case 0b1000: // D
            Position += Right * Velocity;
            break;
        case 0b10000: // SPACE
            Position += WorldUp * Velocity;
            break;
        case 0b100000: // LEFT SHIFT
            Position -= WorldUp * Velocity;
            break;
        default:
            break;
    }
}

void FreeCamera::MouseInput(float xoffset, float yoffset) {
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

void FreeCamera::updateVectors() {
    Front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)); 
    Front.y = sin(glm::radians(Pitch));
    Front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch)); 
    Front = glm::normalize(Front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
