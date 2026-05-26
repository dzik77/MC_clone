cmake_minimum_required(VERSION 3.14)
project(IntroductionGame)

# Use modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependency Management (FetchContent) ---
include(FetchContent)

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ENet (Cross-platform build from source)
FetchContent_Declare(
  enet
  GIT_REPOSITORY https://github.com/lsalzman/enet.git
  GIT_TAG master
)
FetchContent_MakeAvailable(enet)

# GLM (Header-only)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# Find OpenGL and Threads
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# FastNoise2
set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "Build Noise Tool" FORCE)
set(FASTNOISE2_TESTS OFF CACHE BOOL "Build tests" FORCE)

add_subdirectory("deps/FastNoise2")

# --- Project Setup ---

# Add your source files to the executable
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp" "src/*.c")
add_executable(${PROJECT_NAME} ${SRC_FILES})

# Tell the compiler where to find header files
target_include_directories(${PROJECT_NAME} PRIVATE 
    include
)

# Link the libraries to your game
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    glfw
    enet
    FastNoise
    glm::glm
    OpenGL::GL
    Threads::Threads
)

# Platform-specific linking requirements
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 winmm)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif()
