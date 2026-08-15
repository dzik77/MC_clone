#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <PhysicsEngine.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Networking.hpp>

struct BlockUpdate{
    bool isUpdate;
    glm::ivec3 position;
    Block block;
};

class Player : Entity {

    public:

        Player() : Entity() {
            Front = glm::vec3(0.0f,0.0f,-1.0f);
            Right = glm::vec3(1.0f,0.0f,0.0f);
            WorldUp = glm::vec3(0.0f,1.0f,0.0f);
            setPosition(glm::vec3(0.0f, 80.0f, 0.0f));
            onGround = false;
            currentState = {0, glm::vec3(0.0f, 80.0f, 0.0f), -90.0f, 0.0f, 0, 0.0f, 0.0f, 0.0f};
        }
        ~Player() {}

        Camera camera;

        void initialize(IWorldAccessor* worldAccessor) {
            this->worldAccessor = worldAccessor;
        }

        float getYaw() const {
            return Yaw;
        }
        float getPitch() const {
            return Pitch;
        }
        void setRotation(float yaw, float pitch) {
            Yaw = yaw;
            Pitch = pitch;
            camera.updateAngles(Yaw, Pitch);
        }
        void setPosition(glm::vec3 position) {
            this->position = position;
            camera.setPosition(position + glm::vec3(0.0f, 1.62f, 0.0f));
            setBoundingBoxForPlayer(*this);
        }
        glm::vec3 getPosition() const {
            return position;
        }
        glm::vec3 getFront() const {
            return Front;
        }
        SavedClientState getCurrentState() const {
            return currentState;
        }
        BlockUpdate getBlockUpdate() const {
            return blockUpdate;
        }
        void clearBlockUpdate() {
            blockUpdate.isUpdate = false;
        }
        
        void handleInput(uint8_t input);
        void MouseInput(float xoffset, float yoffset);
        void updatePhysics(float deltaTime);
        
    private:
        
        IWorldAccessor* worldAccessor = nullptr;

        SavedClientState currentState;

        BlockUpdate blockUpdate;

        glm::vec3 
            Front,
            Right,
            WorldUp = glm::vec3(0.0f,1.0f,0.0f);

        float 
            MovementSpeed = 100.317f, // orig 4.317f
            JumpSpeed = 30.4f, // orig 8.4f
            MouseSensivity = 0.1f,
            Yaw = -90.0f,
            Pitch = 0.0f;

};