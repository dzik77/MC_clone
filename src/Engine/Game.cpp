#include "Game.hpp"

Game::Game() {
    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet." << std::endl;
        throw std::runtime_error("Failed to initialize ENet.");
    }
    
    glfwSetErrorCallback(error_callback);

#ifdef __linux__
    // Try to use Wayland if available, but don't force it if it fails
    glfwInitHint(GLFW_PLATFORM, GLFW_ANY_PLATFORM); 
#endif

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        enet_deinitialize();
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(800, 600, "Introduction Game", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        enet_deinitialize();
        throw std::runtime_error("Failed to create GLFW window."); 
    }
    std::cout << "GLFW Window created successfully." << std::endl;

    glfwSetWindowUserPointer(window, this);
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 
    glfwSwapInterval(0);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        enet_deinitialize();
        throw std::runtime_error("Failed to initialize GLAD.");
    }

    #ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
    glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
        if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM) {
            std::cerr << "GL CALLBACK: type = " << type << ", severity = " << severity << ", message = " << message << "\n";
        }
    }, nullptr); 
    #endif
    

    std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << " initialized." << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glViewport(0,0,800,600);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Force a buffer swap and event poll to make window visible on some platforms
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();

    stbi_set_flip_vertically_on_load(true);

    world.initialize();
    
}

Game::~Game(){
    world.clean();
    glfwDestroyWindow(window);
    glfwTerminate();
    enet_deinitialize();
}

void Game::run() {

    // shaders
    std::cout << "Loading shaders..." << std::endl;
    Shader shader(vertexShaderPath, fragmentShaderPath);
    std::cout << "Shaders loaded." << std::endl;
    shader.use();
    shader.setInt("textureArray", 0);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.f / 600.f, 0.1f, 1000.f);
    shader.setMat4("projection", projection);
    shader.setMat4("model", glm::mat4(1.f));

    float lastFrame, currentFrame, deltaTime;
    lastFrame = glfwGetTime();
    int frameCount = 0;
    float lastFPSmeasure = lastFrame;

    while(!glfwWindowShouldClose(window)){

        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        frameCount++;
        if (currentFrame - lastFPSmeasure >= 1.0){
            std::string  title = "FPS: " + std::to_string(frameCount);
            glfwSetWindowTitle(window, title.c_str());
            frameCount = 0;
            lastFPSmeasure = currentFrame;
        }

        processInput(deltaTime);
        
        glClearColor(0.2f,0.3f,0.3f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.setMat4("view", camera.getView());

        world.loadChunks(camera.getPosition());
        world.generateMesh();
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
