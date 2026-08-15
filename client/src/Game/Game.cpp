#include <Game/Game.hpp>

Game::Game() {
    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet." << std::endl;
        throw std::runtime_error("Failed to initialize ENet.");
    }
    
    glfwSetErrorCallback(error_callback);

#ifdef __linux__
    // Try to use X11 to avoid Wayland libdecor heap corruption
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11); 
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

#ifdef DEBUG
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
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

    // networking
    networking.initialize(world.receivedChunks, player, world);

    // shaders
    std::cout << "Loading shaders..." << std::endl;
    shader.initialize(vertexShaderPath, fragmentShaderPath);
    std::cout << "Shaders loaded." << std::endl;

    // textures
    loadTextures();

    world.initialize(textureArray.texture);
    
    
    // player
    player.initialize(&world);
    
}

void Game::loadTextures() {
    std::cout << "Loading textures..." << std::endl;
    textureArray.createTextureArray(texturePaths,16,16);
    std::cout << "Textures loaded." << std::endl; 
}

Game::~Game(){

    world.clean();
    textureArray.clean();

#ifdef DEBUG
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
#endif

    glfwDestroyWindow(window);
    glfwTerminate();
    enet_deinitialize();
}

void Game::run() {
    
    std::cout << "Attempting to connect to server..." << std::endl;
    while (!networking.connectToServer("127.0.0.1"));
    std::cout << "Connected to server!" << std::endl;
    world.setServerPeer(networking.getServerPeer());

    shader.use();
    shader.setInt("textureArray", 0);
    projection = glm::perspective(glm::radians(90.0f), 800.f / 600.f, 0.1f, 1000.f);
    shader.setMat4("model", glm::mat4(1.f));

    float lastFrame, currentFrame, deltaTime;
    lastFrame = glfwGetTime();
    int frameCount = 0;
    float lastFPSmeasure = lastFrame;

    world.startMeshGenerationThreads();


    while(!glfwWindowShouldClose(window)){

        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        lastFrame = currentFrame;

        frameCount++;
        if (currentFrame - lastFPSmeasure >= 1.0){
            std::string  title = "FPS: " + std::to_string(frameCount);
            glfwSetWindowTitle(window, title.c_str());
            frameCount = 0;
            lastFPSmeasure = currentFrame;
        }


        //input
        processInput(deltaTime);

        // IMGUI

#ifdef DEBUG
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
#endif

        // render pipeline 

        glClearColor(0.2f,0.3f,0.3f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.setMat4("projection", projection);
        shader.setMat4("view", (!freeCameraMode ? player.camera.getView() : freeCamera.getView()));
        
        world.loadChunks(player.getPosition());
        world.generateMesh();
        world.uploadChunksToGPU();
        world.render(shader, player.getPosition());
        
        // physics
        if (physics && !freeCameraMode){
            player.updatePhysics(deltaTime);
        }
        
        // networking
        networking.sendPlayerMovePacket(player.getCurrentState());
        BlockUpdate blockUpdate = player.getBlockUpdate();
        if (blockUpdate.isUpdate){
            if (blockUpdate.block == Block::AIR){
                networking.sendDestroyBlockPacket(blockUpdate.position);
            } else {
                networking.sendPlaceBlockPacket(blockUpdate.position, blockUpdate.block);
            }
            player.clearBlockUpdate();
        }
        networking.pollEvents();

        // loading chunks from server

        world.loadTerrain();

        // IMGUI rendering
#ifdef DEBUG
        world.renderDebugWindow(player.getPosition());
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
#endif

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    world.stopMeshGenerationThreads();

}

void Game::error_callback(int error, const char *description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

void Game::framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0,0,width,height);
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game && height > 0) {
        game->projection = glm::perspective(glm::radians(90.0f), (float)width / (float)height, 0.1f, 1000.f);
    }
}

void Game::processInput(float deltaTime) {    

    uint8_t input = 0;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        input |= 0b1; // W
    } 
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        input |= 0b10; // S
    } 
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        input |= 0b100; // A
    } 
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        input |= 0b1000; // D
    } 
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
        input |= 0b10000; // SPACE
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
        input |= 0b100000; // LEFT SHIFT
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS){
        input |= 0b1000000; // RIGHT MOUSE BUTTON
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
        input |= 0b10000000; // LEFT MOUSE BUTTON
    }

    if (!freeCameraMode){
        player.handleInput(input);
    } else {
        freeCamera.KeyboardInput(input, deltaTime);
        player.setPosition(freeCamera.getPosition() - glm::vec3(0.0f, 1.62f, 0.0f));
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    } else if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS){
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        focus = false;
    } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        focus = true;
    } else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS){
        physics = !physics;
    } else if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS){
        if (!freeCameraMode){
            freeCamera.setPosition(player.getPosition() + glm::vec3(0.0f, 1.62f, 0.0f));
            freeCamera.setRotation(player.getYaw(), player.getPitch());
            freeCameraMode = true;
        } else {
            player.setPosition(freeCamera.getPosition() - glm::vec3(0.0f, 1.62f, 0.0f));
            player.setRotation(freeCamera.getYaw(), freeCamera.getPitch());
            freeCameraMode = false;
        }
    }
}

void Game::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;

    static bool firstCallback = true;
    if (game->focus) {
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
        
        if (game->freeCameraMode){
            game->freeCamera.MouseInput(xoffset, yoffset);
        } else {
            game->player.MouseInput(xoffset, yoffset);
        }
    } else {
        firstCallback = true;
    }


}
