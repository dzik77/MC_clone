#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Direction.hpp> 

class FreeCamera{
    public:

        FreeCamera() {
            Position = glm::vec3(0.0f,20.0f,3.0f);
            Front = glm::vec3(0.0f,0.0f,-1.0f);
            Up = glm::vec3(0.0f,1.0f,0.0f);
            Right = glm::vec3(1.0f,0.0f,0.0f);
            WorldUp = glm::vec3(0.0f,1.0f,0.0f);
            updateVectors();
        }
        inline glm::mat4 getView(){
            return glm::lookAt(Position, Position + Front, Up);
        }
        void KeyboardInput(Direction direction, float deltaTime);
        void MouseInput(float xoffset, float yoffset);
        glm::vec3 getPosition(){
            return Position;
        }

    private:

        float 
            MovementSpeed = 200.5f,
            MouseSensivity = 0.1f,
            Yaw = -90.0f,
            Pitch = 0.0f;

        glm::vec3
            Position,
            Front,
            Up,
            Right,
            WorldUp;  

        void updateVectors();

};