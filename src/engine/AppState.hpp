#pragma once
#ifndef _APP_STATE_HPP_
#define _APP_STATE_HPP_

#include "core/Scene.hpp"
#include "engine/camera.hpp"
#include "engine/Environment.hpp"
#include "engine/render/Renderer.hpp"
#include "engine/render/RenderContext.hpp"
#include "engine/render/CommandQueue.hpp"
#include "engine/render/CommandDebugQueue.hpp"
#include "engine/visibility/FrustumCulling.hpp"
#include "system/RenderStats.hpp"

namespace Long {
    //AppState base class
    class AppState {
    public:
        virtual ~AppState() = default;
        virtual void OnEnter() {}
        virtual void OnExit() {}
        virtual void Update(float dt) {}
        virtual void RenderWorld() {}
        virtual void RenderUI() {}
    };

    class ContextSharedStateRenderer {
    public:
        virtual ~ContextSharedStateRenderer() = default;
        virtual Scene& getSharedScene() = 0;
        void RenderShared() {
            RenderContext ctx;
            ctx.commandQueue = &m_commandQueue;
            ctx.commandDebugQueue = &m_commandDebugQueue;
            ctx.environment = &m_environment;
            ctx.registry = &getSharedScene().Registry();
            ctx.camera = &m_camera.Raw();
            ctx.frustum = &m_frustum;
            ctx.width = (uint32_t)GetRenderWidth();
            ctx.height = (uint32_t)GetRenderHeight();
            decorateContext(ctx);     
            m_renderer.Render(ctx);
            m_renderStats = ctx.renderStats;
        }

    protected:
        virtual void decorateContext(RenderContext& ctx) {}
        Environment m_environment;
        CommandQueue m_commandQueue;
        CommandDebugQueue m_commandDebugQueue;
        EditorCamera m_camera;
        Renderer m_renderer;
        FrustumCulling m_frustum;
        RenderStats m_renderStats;
    };
}
#endif // !_APP_STATE_HPP_
