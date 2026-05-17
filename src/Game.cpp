#include "Game.hpp"

Game::Game() {
    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet." << std::endl;
        throw std::runtime_error("Failed to initialize ENet.");
    }
    
    glfwSetErrorCallback(error_callback);
#ifdef __linux__
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND); 
#endif
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        enet_deinitialize();
        throw std::runtime_error("Failed to initialize GLFW.");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(800, 600, "Introduction Game", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        enet_deinitialize();
        throw std::runtime_error("Failed to create GLFW window."); 
    }
    glfwSetWindowUserPointer(window, this);
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        enet_deinitialize();
        throw std::runtime_error("Failed to initialize GLAD.");
    }
    std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << " initialized." << std::endl;

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glViewport(0,0,800,600);
}

Game::~Game() {
    glfwDestroyWindow(window);
    glfwTerminate();
    enet_deinitialize();
}

void Game::run() {

    // shaders
    Shader shader(vertexShaderPath, fragmentShaderPath);
    shader.use();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.f / 600.f, 0.1f, 100.f);
    shader.setMat4("projection", projection);
    shader.setMat4("model", glm::mat4(1.f));

    float lastFrame, currentFrame, deltaTime;
    lastFrame = glfwGetTime();

    while(!glfwWindowShouldClose(window)){

        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(deltaTime);
        
        shader.setMat4("view", camera.getView());

        world.loadChunks(camera.getPosition());
        world.render(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Game::error_callback(int error, const char *description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

void Game::framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0,0,width,height);
}

void Game::processInput(float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        camera.KeyboardInput(Direction::FORWARD, deltaTime);
    } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        camera.KeyboardInput(Direction::BACKWARD, deltaTime);
    } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        camera.KeyboardInput(Direction::LEFT, deltaTime);
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        camera.KeyboardInput(Direction::RIGHT, deltaTime);
    } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS){
        camera.KeyboardInput(Direction::UP, deltaTime);
    } else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS){
        camera.KeyboardInput(Direction::DOWN, deltaTime);
    }
}

void Game::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;

    static bool firstCallback = true;
    static double lastX, lastY;

    if (firstCallback){
        lastX = xpos;
        lastY = ypos;
        firstCallback = false;
    }
    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos;
    lastX = xpos;
    lastY =  ypos;

    game->camera.MouseInput(xoffset, yoffset);

}
