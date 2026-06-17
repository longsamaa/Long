#include "editor/EditorState.hpp"
#include "engine/Application.hpp"
#include "editor/panels/GpuInfoPanel.hpp"
#include "editor/panels/ProfilerPanel.hpp"
#include "editor/panels/ConsolePanel.hpp"
#include "core/Components.hpp"
#include "system/RenderSystem.hpp"
#include "system/TransformSystem.hpp"
#include "imgui.h"
#include "raylib-cpp.hpp"


namespace Long {
    void EditorState::OnEnter() {
        m_panels.push_back(std::make_unique<GpuInfoPanel>());
        m_panels.push_back(std::make_unique<ProfilerPanel>(m_renderStats));
        m_panels.push_back(std::make_unique<ConsolePanel>());
        testCreateDefaultCube();
    }

    void EditorState::Update(float dt) {
        (void)dt;
        // Recompute world transforms from the hierarchy before rendering.
        TransformSystem(m_scene.Registry());
    }

    void EditorState::RenderWorld() {
        m_camera.Begin3D();
        DrawGrid(20, 1.0f); // ground reference grid
        RenderSystem(m_scene.Registry(), m_app.GetAssets(), m_renderStats);
        m_camera.End3D();
    }

    void EditorState::testCreateDefaultCube()
    {
        auto& assets = m_app.GetAssets();
        raylib::Mesh cube = raylib::Mesh::Cube(1.0f, 1.0f, 1.0f);
        uint32_t meshId = assets.AddMesh(std::move(cube));

        // Use the "default" shader loaded by Application; build a flat-color
        // material on top of it.
        uint32_t shaderId = assets.GetShaderId("default");
        uint32_t matId = assets.CreateDefaultMaterial(shaderId, raylib::Color::Maroon());
        auto& reg = m_scene.Registry();
        for (int i = 0; i < 3; ++i) {
            entt::entity e = m_scene.CreateEntity("cube");
            Transform t;
            t.position = { i * 2.0f - 2.0f, 0.5f, 0.0f };
            t.scale = { 1.0f, 1.0f, 1.0f };
            reg.emplace<Transform>(e, t);
            reg.emplace<MeshFilter>(e, MeshFilter{ meshId });
            reg.emplace<MeshRenderer>(e, MeshRenderer{ matId, raylib::Color::Maroon(), true });
        }
    }

    void EditorState::RenderMenuBar()
    {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Quit")) {
                    m_app.Quit();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Window")) {
                for (auto& panel : m_panels) {
                    ImGui::MenuItem(panel->title().c_str(), nullptr, &panel->isOpen());
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void EditorState::RenderPanels()
    {
        for (auto& panel : m_panels) {
            panel->render();
        }
    }
    
    void EditorState::RenderUI() {
        // Menu bar.
        RenderMenuBar(); 
        // Each panel decides whether to draw based on its own open flag.
        RenderPanels(); 
    }
    
}
