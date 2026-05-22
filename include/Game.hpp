#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <enet/enet.h>
#include <iostream>
#include "Shaders.hpp"
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Camera.hpp>
#include <World.hpp>
#include <string>
#include <stb_image.h>

#define DEBUG

class Game{
    public:
        Game();
        ~Game();
        void run();
    private:

        Camera camera;
        GLFWwindow* window;
        World world;
        const std::filesystem::path vertexShaderPath{"Shaders/vertex.glsl"};
        const std::filesystem::path fragmentShaderPath{"Shaders/fragment.glsl"};

        void processInput(float deltaTime);
        static void error_callback(int error, const char* description);
        static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
        static void mouse_callback(GLFWwindow* Window, double xpos, double ypos);

};  