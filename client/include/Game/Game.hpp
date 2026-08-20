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
#include <Engine/FreeCamera.hpp>
#include <Engine/TextureManager.hpp>
#include <PhysicsEngine.hpp>

// DEBUGGING

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// #define DEBUG handled by CMake
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
        const std::filesystem::path vertexWorldShaderPath{"Shaders/vertexWorld.glsl"};
        const std::filesystem::path fragmentWorldShaderPath{"Shaders/fragmentWorld.glsl"};
        const std::filesystem::path vertexPlayerShaderPath{"Shaders/vertexPlayer.glsl"};
        const std::filesystem::path fragmentPlayerShaderPath{"Shaders/fragmentPlayer.glsl"};
        Shader shaderWorld, shaderPlayer;

        //Texture
        TextureArray textureArray;
        void loadTextures();
        const std::vector<std::filesystem::path> texturePaths = {
            "textures/dirt.png",
            "textures/grass_block_side.png",
            "textures/grass_block_top_item.png",
            "textures/stone.png",
            "textures/water_still.png",
            "textures/log.png",
            "textures/top_log.png",
            "textures/leaves.png"
        };

        Networking networking;

        // Main Player
        Player player;
        FreeCamera freeCamera;
        bool freeCameraMode = false;

        // Camera view modes (F5)
        enum class ViewMode { FirstPerson, ThirdPersonBack, ThirdPersonFront };
        ViewMode viewMode = ViewMode::FirstPerson;
        bool wasF5Pressed = false;

        static constexpr float THIRD_PERSON_DISTANCE = 4.0f;
        static constexpr float THIRD_PERSON_COLLISION_MARGIN = 0.2f;
        static constexpr float THIRD_PERSON_MIN_DISTANCE = 0.1f;

        glm::mat4 getActiveView();

        //physics
        bool physics = true;

        // ui inputs
        bool focus = true;

        void processInput(float deltaTime);
        static void error_callback(int error, const char* description);
        static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
        static void mouse_callback(GLFWwindow* Window, double xpos, double ypos);
        
};  
