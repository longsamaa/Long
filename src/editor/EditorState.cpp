#include "editor/EditorState.h"
#include "engine/Application.h"
#include "editor/panels/GpuInfoPanel.hpp"
#include "imgui.h"


namespace Long {
    void EditorState::OnEnter() {
        m_panels.push_back(std::make_unique<GpuInfoPanel>());
    }

    void EditorState::Update(float dt) {
        (void)dt;
    }

    void EditorState::RenderWorld() {
        // Placeholder so we can see the loop works. Will become the 3D grid view.
        auto& w = m_app.GetWindow();
        raylib::Vector2 center(w.GetWidth() / 2.0f, w.GetHeight() / 2.0f);
        center.DrawCircle(w.GetHeight() * 0.4f, DARKGREEN);
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
