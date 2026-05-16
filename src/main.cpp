#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <enet/enet.h>
#include <iostream>
#include "Shaders.hpp"
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const std::filesystem::path vertexShaderPath("Shaders/vertex.glsl");
const std::filesystem::path fragmentShaderPath("Shaders/fragment.glsl");

void error_callback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

float vertices[] = {
        // positions          // colors
         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f, // top right
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f  // top left
};
unsigned int indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
};

int main(){

    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet." << std::endl;
        return EXIT_FAILURE;
    }

    glfwSetErrorCallback(error_callback);
#ifdef __linux__
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND); 
#endif
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return EXIT_FAILURE;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Introduction Game", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        enet_deinitialize();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        enet_deinitialize();
        return EXIT_FAILURE;
    }
    std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << " initialized." << std::endl;


    // data
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // shaders
    Shader shader(vertexShaderPath, fragmentShaderPath);

    while(!glfwWindowShouldClose(window)){

        glClearColor(0.2f,0.3f,0.3f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    enet_deinitialize();
    
    return 0;
}