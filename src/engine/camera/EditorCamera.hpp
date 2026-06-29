#pragma once
#ifndef _EDITOR_CAMERA_HPP_
#define _EDITOR_CAMERA_HPP_
#include "BaseCamera.hpp"
#include "raylib-cpp.hpp"
namespace Long {
	class EditorCamera : public BaseCamera {
	public:
		EditorCamera();
		void Update(float dt) override;
		void ZoomToward(float wheel, const raylib::Vector3& pivot);
		void FocusOn(const raylib::Vector3& point);
		void BeginMode() override { ApplyClip(); m_camera.BeginMode(); };
		void EndMode() override { m_camera.EndMode(); }
		const raylib::Camera3D& Raw() const override { return m_camera; }
	private:
		void UpdateCameraVectors();
		raylib::Camera3D m_camera;
		raylib::Vector3 m_target = { 0.0f, 0.0f, 0.0f }; // point the camera looks at
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