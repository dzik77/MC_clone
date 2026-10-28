cmake_minimum_required(VERSION 3.14)
project(IntroductionGame)

# Use modern C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Release build if none specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Release optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

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
target_include_directories(enet PUBLIC ${enet_SOURCE_DIR}/include)

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

# ImGui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
  add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  )
  target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

  # ImGui Backends
  add_library(imgui_backends STATIC
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  )
  target_include_directories(imgui_backends PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(imgui_backends PUBLIC imgui glfw OpenGL::GL)
endif()

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
    target_link_libraries(core_lib PUBLIC enet glm::glm FastNoise imgui)
else()
    # Dummy target if empty so we don't break CMake before you move files
    add_library(core_lib INTERFACE)
    target_include_directories(core_lib INTERFACE core/include)
    target_link_libraries(core_lib INTERFACE enet glm::glm FastNoise imgui)
endif()

# 2. Server Executable
file(GLOB_RECURSE SERVER_SRC CONFIGURE_DEPENDS "server/src/*.cpp" "server/src/*.c")
if(SERVER_SRC)
    add_executable(server_app ${SERVER_SRC})
    target_include_directories(server_app PRIVATE server/include)
    #target_compile_definitions(server_app PRIVATE PERFORMANCE_DEBUG)
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
    #target_compile_definitions(client_app PRIVATE DEBUG)
    target_link_libraries(client_app PRIVATE core_lib glfw OpenGL::GL Threads::Threads imgui_backends)
    if(WIN32)
        target_link_libraries(client_app PRIVATE ws2_32 winmm)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(client_app PRIVATE dl)
    endif()

    # --- Runtime assets: keep Shaders/ and textures/ next to the binary ---
    # Re-copied automatically whenever any asset file changes (stamp-guarded so
    # untouched builds do no work). CONFIGURE_DEPENDS re-runs CMake when files
    # are added or removed.
    # Pin the binary location so the assets can be staged beside it without
    # depending on client_app itself (that would be a dependency cycle).
    set(CLIENT_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    set_target_properties(client_app PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "$<1:${CLIENT_RUNTIME_DIR}>")

    file(GLOB_RECURSE CLIENT_ASSETS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/client/Shaders/*"
        "${CMAKE_CURRENT_SOURCE_DIR}/client/textures/*")

    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.26)
        set(COPY_DIR_CMD copy_directory_if_different)
    else()
        set(COPY_DIR_CMD copy_directory)
    endif()

    set(CLIENT_ASSETS_STAMP "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/client_assets.stamp")
    add_custom_command(
        OUTPUT ${CLIENT_ASSETS_STAMP}
        COMMAND ${CMAKE_COMMAND} -E ${COPY_DIR_CMD}
                "${CMAKE_CURRENT_SOURCE_DIR}/client/Shaders"
                "${CLIENT_RUNTIME_DIR}/Shaders"
        COMMAND ${CMAKE_COMMAND} -E ${COPY_DIR_CMD}
                "${CMAKE_CURRENT_SOURCE_DIR}/client/textures"
                "${CLIENT_RUNTIME_DIR}/textures"
        COMMAND ${CMAKE_COMMAND} -E touch ${CLIENT_ASSETS_STAMP}
        DEPENDS ${CLIENT_ASSETS}
        COMMENT "Copying Shaders and textures to build directory"
        VERBATIM)

    add_custom_target(client_assets DEPENDS ${CLIENT_ASSETS_STAMP})
    add_dependencies(client_app client_assets)

endif()
