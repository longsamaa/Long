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
        virtual void OnEnter() {}
        virtual void OnExit() {}
        virtual void Update(float dt) {}
        virtual void RenderWorld() {}
        virtual void RenderUI() {}
        virtual void Execute(RenderContext& ctx) = 0;
    protected: 
        State m_state{ State::GAME };
    };
}
#endif // !_APP_STATE_HPP_
