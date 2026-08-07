#pragma once
#include <glm/glm.hpp>

class Physics {

    public:

        Physics() : deltaTime(0.0f) {}
        ~Physics() {}

        void update(float dt);
        void updateEntityPosition();

    private:

        float gravity = -9.81f; // m/s^2
        float deltaTime;

};