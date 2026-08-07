#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Direction.hpp> 

class Camera{
    public:

        Camera() {
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
        void updateAngles(float Yaw, float Pitch){
            this->Yaw = Yaw;
            this->Pitch = Pitch;
            updateVectors();
        }
        void setPosition(glm::vec3 position){
            Position = position;
        }
        void updatePosition(glm::vec3 delta){
            Position += delta;
        }
        glm::vec3 getPosition(){
            return Position;
        }

    private:

        float 
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