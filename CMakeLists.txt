cmake_minimum_required(VERSION 3.21)

project(long LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# LongEngineCore is a STATIC library. A DLL was tried but raylib (linked statically
# into it) keeps its OpenGL function pointers in global state; splitting that across
# a DLL boundary left gl* pointers null in the exe -> crash in glVertexAttribPointer.
# Static keeps a single copy of raylib's state in the final exe, which is robust.

# ---- raylib (C library) from git submodule, built statically into the app ----
# Building from source with raylib's own bundled GLFW avoids the DLL / external
# GLFW event-loop hang we hit with the prebuilt vcpkg DLL.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
# Use OpenGL 4.3 -- the newest version raylib supports (enables compute shaders
# / SSBOs). GLSL shaders then target #version 430.
set(OPENGL_VERSION "4.3" CACHE STRING "" FORCE)
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

# ============================================================================
#  LongEngineCore  --  the reusable core (ECS, render pipeline, math, assets).
#  Everything under src/ EXCEPT src/editor/ and src/main.cpp. Built as a static
#  library so different front-ends (the editor below, a future Android "player")
#  can all link the same engine. The editor-only ImGui bits still live in
#  Application for now, so the lib links ImGui too; that dependency can be lifted
#  later when porting to platforms without ImGui.
# ============================================================================
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.frag"
)
# Drop editor sources and the executables' entry points from the engine library.
# (src/game/ stays in the core -- it's runtime, reused by editor Play mode and the
# standalone game; only the game's main() is excluded here.)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/src/editor/")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/src/main\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/src/game_main\\.cpp$")

add_library(LongEngineCore STATIC ${ENGINE_SOURCES})
# PUBLIC include dir + deps: anything linking LongEngineCore inherits these.
target_include_directories(LongEngineCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(LongEngineCore PUBLIC raylib raylib_cpp imgui rlimgui entt)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${ENGINE_SOURCES})

# ============================================================================
#  LongEditor  --  the editor executable. Editor-only code + entry point, linked
#  against LongEngineCore. A future "LongPlayer" target would link LongEngineCore
#  too but bring its own (touch/gamepad) front-end instead of src/editor.
# ============================================================================
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/editor/*.hpp"
)
add_executable(LongEditor ${EDITOR_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
target_link_libraries(LongEditor PRIVATE LongEngineCore)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src"
    FILES ${EDITOR_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

set_target_properties(LongEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ============================================================================
#  LongGame  --  the shipped game executable. No editor; boots straight into the
#  Game runtime state. Same LongEngineCore, different entry point. This is the
#  target you'd cross-compile for Android later.
# ============================================================================
add_executable(LongGame "${CMAKE_CURRENT_SOURCE_DIR}/src/game_main.cpp")
target_link_libraries(LongGame PRIVATE LongEngineCore)
set_target_properties(LongGame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
# The game needs the same shaders/resources beside its exe.
add_custom_command(TARGET LongGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders" "$<TARGET_FILE_DIR:LongGame>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resources" "$<TARGET_FILE_DIR:LongGame>/resources"
    COMMENT "Copying shaders/resources -> $<TARGET_FILE_DIR:LongGame>")

# ---- Copy shaders next to the executable after each build ----
# Shaders are loaded at runtime by relative path, so they must sit beside the
# exe. $<TARGET_FILE_DIR:long> resolves to the real output dir (handles the
# Debug/Release subfolder that Visual Studio adds). copy_directory_if_different
# only copies changed files, so it's cheap to run every build.
set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders")
add_custom_command(TARGET LongEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${SHADER_SRC_DIR}" "$<TARGET_FILE_DIR:LongEditor>/shaders"
    COMMENT "Copying shaders -> $<TARGET_FILE_DIR:LongEditor>/shaders")

# ---- Copy resources (textures, models, ...) next to the executable ----
# Same idea as shaders: loaded at runtime by relative path, copied per build.
set(RESOURCES_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")
add_custom_command(TARGET LongEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${RESOURCES_SRC_DIR}" "$<TARGET_FILE_DIR:LongEditor>/resources"
    COMMENT "Copying resources -> $<TARGET_FILE_DIR:LongEditor>/resources")
