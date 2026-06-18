#pragma once
#ifndef _EDITOR_CAMERA_HPP_
#define _EDITOR_CAMERA_HPP_

#include "raylib-cpp.hpp"

namespace Long {

// Unity Scene-view style orbit camera. State is stored as an orbit (target +
// distance + yaw/pitch); the raylib Camera3D position is derived from it.
//
// Controls (handled in Update):
//   Middle mouse drag  -> pan (move target on the view plane)
//   Right mouse drag   -> orbit (rotate around target)
//   Mouse wheel        -> zoom (change distance)
class EditorCamera {
public:
    EditorCamera();

    // Process input and recompute the camera. dt unused for now (input is
    // frame-delta based) but kept for future smoothing.
    void Update(float dt);

    void Begin3D() { m_camera.BeginMode(); }
    void End3D()   { m_camera.EndMode(); }

    raylib::Camera3D& Raw() { return m_camera; }

private:
    // Recompute m_camera.position/target from the orbit parameters.
    void UpdateCameraVectors();

    raylib::Camera3D m_camera;

    raylib::Vector3 m_target = {0.0f, 0.0f, 0.0f}; // point the camera looks at
    float m_distance = 18.0f;                       // distance from target
    float m_yaw = -45.0f;                           // horizontal angle (deg)
    float m_pitch = 45.0f;                          // vertical angle (deg)

    // Tunables.
    float m_orbitSpeed = 0.3f;   // deg per pixel
    float m_panSpeed = 0.001f;    // world units per pixel (scaled by distance)
    float m_zoomSpeed = 1.5f;    // distance per wheel notch
    float m_minDistance = 2.0f;
    float m_maxDistance = 100.0f;
    float m_minPitch = -89.0f;
    float m_maxPitch = 89.0f;
};

} // namespace Long

#endif // !_EDITOR_CAMERA_HPP_
