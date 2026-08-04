cmake_minimum_required(VERSION 3.14)
project(IntroductionGame)

# Use modern C++
set(CMAKE_CXX_STANDARD 20)
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

# --- Project Targets ---

# 1. Core Library (Shared)
file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS "core/src/*.cpp" "core/src/*.c")
# Check if any sources were found to avoid CMake errors if empty
if(CORE_SRC)
    add_library(core_lib STATIC ${CORE_SRC})
    target_include_directories(core_lib PUBLIC core/include)
    target_link_libraries(core_lib PUBLIC enet glm::glm FastNoise)
else()
    # Dummy target if empty so we don't break CMake before you move files
    add_library(core_lib INTERFACE)
    target_include_directories(core_lib INTERFACE core/include)
    target_link_libraries(core_lib INTERFACE enet glm::glm FastNoise)
endif()

# 2. Server Executable
file(GLOB_RECURSE SERVER_SRC CONFIGURE_DEPENDS "server/src/*.cpp" "server/src/*.c")
if(SERVER_SRC)
    add_executable(server_app ${SERVER_SRC})
    target_include_directories(server_app PRIVATE server/include)
    target_link_libraries(server_app PRIVATE core_lib Threads::Threads)
    if(WIN32)
        target_link_libraries(server_app PRIVATE ws2_32 winmm)
    endif()
endif()

# 3. Client Executable
file(GLOB_RECURSE CLIENT_SRC CONFIGURE_DEPENDS "client/src/*.cpp" "client/src/*.c")
if(CLIENT_SRC)
    add_executable(client_app ${CLIENT_SRC})
    target_include_directories(client_app PRIVATE client/include)
    target_link_libraries(client_app PRIVATE core_lib glfw OpenGL::GL Threads::Threads)
    if(WIN32)
        target_link_libraries(client_app PRIVATE ws2_32 winmm)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(client_app PRIVATE dl)
    endif()
endif()
