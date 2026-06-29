#pragma once
#ifndef _APP_STATE_HPP_
#define _APP_STATE_HPP_
#include "core/Scene.hpp"
#include "engine/render/RenderContext.hpp"
namespace Long {
    //AppState base class
    enum class State {
        EDITOR,
        GAME
    };
    class AppState {
    public:
        virtual ~AppState() = default;
        virtual void OnEnter() = 0; 
        virtual void OnExit() = 0; 
        virtual void Update(float dt) = 0; 
        virtual void BeginFrame() = 0; 
        virtual void RenderWorld() = 0; 
        virtual void RenderUI() = 0; 
        virtual void EndFrame() = 0; 
        virtual void Execute(RenderContext& ctx) = 0;
    protected: 
        State m_state{ State::GAME };
    };
}
#endif // !_APP_STATE_HPP_
