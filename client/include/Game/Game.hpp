#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <enet/enet.h>
#include <iostream>
#include <Engine/Shaders.hpp>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Engine/Camera.hpp>
#include <WorldGeneration/World.hpp>
#include <string>
#include <Engine/stb_image.h>
#include <Engine/Networking.hpp>
#include <Engine/Player.hpp>

#define DEBUG
//#define AZDO_RENDERING

class Game{
    public:

        Game();
        ~Game();
        void run();

    private:

        GLFWwindow* window;
        World world;
        glm::mat4 projection;
        const std::filesystem::path vertexShaderPath{"Shaders/vertex.glsl"};
        const std::filesystem::path fragmentShaderPath{"Shaders/fragment.glsl"};
        Shader shader;

        Networking networking;

        // Main Player
        Player player;

        //physics
        bool physics = true;

        // ui inputs
        bool focus = true;

        void processInput(float deltaTime);
        static void error_callback(int error, const char* description);
        static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
        static void mouse_callback(GLFWwindow* Window, double xpos, double ypos);
        
};  
