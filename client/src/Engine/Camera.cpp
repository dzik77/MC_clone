#include <Engine/Camera.hpp>

void Camera::updateVectors() {
    Front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch)); 
    Front.y = sin(glm::radians(Pitch));
    Front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch)); 
    Front = glm::normalize(Front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}
