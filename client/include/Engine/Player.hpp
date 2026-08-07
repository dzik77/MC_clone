#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <PhysicsEngine.hpp>
#include <Engine/Camera.hpp>

class Player : Entity {

    public:

        Player() : Entity() {
            Front = glm::vec3(0.0f,0.0f,-1.0f);
            Right = glm::vec3(1.0f,0.0f,0.0f);
            WorldUp = glm::vec3(0.0f,1.0f,0.0f);
            setPosition(glm::vec3(0.0f, 100.0f, 0.0f));
            onGround = false;
        }
        ~Player() {}

        Camera camera;

        void setPosition(glm::vec3 position) {
            this->position = position;
            camera.setPosition(position + glm::vec3(0.0f, 1.62f, 0.0f));
            boundingBox.min = position - glm::vec3(0.3f, 0.0f, 0.3f);
            boundingBox.max = position + glm::vec3(0.3f, 1.8f, 0.3f);
        }
        glm::vec3 getPosition() const {
            return position;
        }
        glm::vec3 getFront() const {
            return Front;
        }
        
        void handleInput(uint8_t input);
        void MouseInput(float xoffset, float yoffset);
        void updatePhysics(float deltaTime, const IWorldAccessor& worldAccessor);
        
    private:

        glm::vec3 
            Front,
            Right,
            WorldUp = glm::vec3(0.0f,1.0f,0.0f);

        float 
            MovementSpeed = 4.317f,
            JumpSpeed = 8.4f,
            MouseSensivity = 0.1f,
            Yaw = -90.0f,
            Pitch = 0.0f;

};