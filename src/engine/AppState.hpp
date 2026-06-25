#pragma once
#ifndef _APP_STATE_HPP_
#define _APP_STATE_HPP_
//AppState base class
namespace Long {
    class AppState {
    public:
        virtual ~AppState() = default;
        // Called once when this state becomes active / inactive.
        virtual void OnEnter() {}
        virtual void OnExit() {}
        // Per-frame logic. dt = seconds since last frame.
        virtual void Update(float dt) {}
        // Draw the raylib scene (cleared background, 3D world, etc.).
        virtual void RenderWorld() {}
        // Draw ImGui UI on top of the world.
        virtual void RenderUI() {}
        // Report the previous frame's EndDrawing/SwapBuffers time (ms), measured by
        // Application after RenderUI. Lets a state surface it in its profiler.
        virtual void ReportEndDrawingMs(double) {}
    };
}
#endif // !_APP_STATE_HPP_


