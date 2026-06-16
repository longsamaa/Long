#pragma once
#ifndef _EDITOR_STATE_HPP_
#define _EDITOR_STATE_HPP_
#include "engine/AppState.hpp"
#include "engine/camera.hpp"
#include "core/Scene.hpp"
#include "editor/IPanel.hpp"
#include "system/RenderStats.hpp"
#include <memory>
#include <vector>
namespace Long {
    class Application;
    // The level editor mode. Owns the camera, the level being edited and the
    // editor panels. For now it's a skeleton that draws a placeholder scene + UI.
    class EditorState : public AppState {
    public:
        explicit EditorState(Application& app) : m_app(app) {}
        void OnEnter() override;
        void Update(float dt) override;
        void RenderWorld() override;
        void RenderUI() override;
    private: 
        void testCreateDefaultCube(); 
        void RenderMenuBar();
        void RenderPanels(); 
    private:
        Application& m_app;
        std::vector<std::unique_ptr<IPanel>> m_panels;
        Scene m_scene;
        EditorCamera m_camera;
        RenderStats m_renderStats;
    };
}
#endif // !_EDITOR_STATE_HPP_






