#pragma once
#ifndef _EDITOR_CAMERA_HPP_
#define _EDITOR_CAMERA_HPP_

#include "raylib-cpp.hpp"

namespace Long {

// A top-down-ish 3D camera looking at a target on the ground, tilted like the
// reference screenshot. Begin3D()/End3D() wrap raylib's mode so the render
// system can draw the world in between.
class EditorCamera {
public:
    EditorCamera();

    void Begin3D() { m_camera.BeginMode(); }
    void End3D()   { m_camera.EndMode(); }

    raylib::Camera3D& Raw() { return m_camera; }

private:
    raylib::Camera3D m_camera;
};

} // namespace Long

#endif // !_EDITOR_CAMERA_HPP_
