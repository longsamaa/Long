#pragma once
#ifndef _EDITOR_STATE_HPP_
#define _EDITOR_STATE_HPP_
#include "engine/AppState.h"
#include "editor/IPanel.hpp"
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
        void RenderMenuBar();
        void RenderPanels(); 
    private:
        Application& m_app;
        std::vector<std::unique_ptr<IPanel>> m_panels;
    };
}
#endif // !_EDITOR_STATE_HPP_






