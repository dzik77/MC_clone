#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <PhysicsEngine.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Networking.hpp>
#include <glad/glad.h>

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
            mesh = Mesh::buildHumanoid();
        }
        ~Player() {
            mesh.destroy();
        }

        Camera camera;

        void initialize(IWorldAccessor* worldAccessor) {
            this->worldAccessor = worldAccessor;
            mesh.create();
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

        struct Vertex {
            glm::vec3 position;
            glm::vec2 texCoords;
            int32_t packed; // (partID << 3) | faceID)
        };
        enum class BodyPart : int32_t {
            Head = 0, Torso, RightArm, LeftArm, RightLeg, LeftLeg, Count
        };
        struct Mesh {
            std::vector<Vertex> vertices;
            std::vector<uint16_t> indices;
            GLuint VAO,VBO,EBO;

            void create();
            void render();
            void destroy();

            static constexpr float TEX_SIZE  = 64.0f;  // skin is 64x64
            static constexpr float PX_PER_M  = 16.0f;  // 16 pixels per block

            // size    : box dimensions in pixels (w, h, d)
            // origin  : min corner relative to this part's pivot, in pixels
            // uvOffset: top-left of this box's region in the skin, in pixels
            // inflate : 0.0 for base layer, 0.5 for hat/jacket overlay
            void addBox(BodyPart part, glm::vec3 size, glm::vec3 origin,
                        glm::vec2 uvOffset, float inflate = 0.0f);

            static Mesh buildHumanoid(bool withOverlay = false);
        };
                
        Mesh mesh;

    private:
        
        IWorldAccessor* worldAccessor = nullptr;

        SavedClientState currentState;

        BlockUpdate blockUpdate;

        glm::vec3 
            Front,
            Right,
            WorldUp = glm::vec3(0.0f,1.0f,0.0f);

        float 
            MovementSpeed = 4.317f, // orig 4.317f
            JumpSpeed = 8.4f, // orig 8.4f
            MouseSensivity = 0.1f,
            Yaw = -90.0f,
            Pitch = 0.0f;

};