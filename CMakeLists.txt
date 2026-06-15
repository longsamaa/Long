cmake_minimum_required(VERSION 3.21)

project(long LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- raylib (C library) from git submodule, built statically into the app ----
# Building from source with raylib's own bundled GLFW avoids the DLL / external
# GLFW event-loop hang we hit with the prebuilt vcpkg DLL.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/raylib)

# ---- raylib-cpp (C++ wrapper, git submodule) ----
# Header-only INTERFACE target `raylib_cpp`.
set(BUILD_RAYLIB_CPP_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/raylib-cpp)

# ---- EnTT (ECS, git submodule) ----
# Header-only. We don't add_subdirectory (it would pull in tests / sanitizers);
# just expose the single-header include dir as an INTERFACE target.
add_library(entt INTERFACE)
target_include_directories(entt INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/entt/single_include")

# ---- Dear ImGui (git submodule) ----
# No CMakeLists shipped; compile core sources into a static lib. Everything is
# static, so there is a single ImGui context linked into the exe.



set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR})

# ---- rlImGui (raylib backend for ImGui, git submodule) ----
set(RLIMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/rlImGui")
add_library(rlimgui STATIC
    ${RLIMGUI_DIR}/rlImGui.cpp
)
target_include_directories(rlimgui PUBLIC ${RLIMGUI_DIR})
target_link_libraries(rlimgui PUBLIC imgui raylib)

# ---- App sources (our own code under src/) ----
# GLOB our code because we add files often. CONFIGURE_DEPENDS makes CMake
# re-run the glob when files are added/removed, so you don't have to
# reconfigure manually. (We only glob .cpp -- headers are found via include
# dirs. We also glob headers (.h/.hpp) and add them to the target so they show
# up in IDEs (Visual Studio / VS Code) -- headers are not compiled, just listed.
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

# ---- App ----
add_executable(long ${APP_SOURCES})
target_include_directories(long PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(long PRIVATE raylib raylib_cpp imgui rlimgui entt)

# Show the src/ folder tree (core/engine/editor/game) as filters in Visual
# Studio instead of a flat "Source Files" list.
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${APP_SOURCES})

set_target_properties(long PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
