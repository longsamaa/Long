#pragma once
#ifndef _DRAW_DEBUG_HELPER_HPP_
#define _DRAW_DEBUG_HELPER_HPP_

#include "raylib-cpp.hpp"
#include "engine/camera/BaseCamera.hpp"
#include "engine/render/CommandDebugQueue.hpp"

namespace Long {
	raylib::BoundingBox MakeWorldBoundingBox(const raylib::BoundingBox& localBox,
		const raylib::Matrix& world);

	void DrawCircle3D(raylib::Vector3 center, raylib::Vector3 u, raylib::Vector3 v,
		float radius, raylib::Color color, float thickness = 1.0f,
		int segments = 48);

	void DrawRing3D(raylib::Vector3 center, raylib::Vector3 u, raylib::Vector3 v,
		float radius, float width, raylib::Color color, int segments = 48);

	void DrawTorus3D(raylib::Vector3 center, raylib::Vector3 u, raylib::Vector3 v,
		float radius, float tube, raylib::Color color,
		int ringSegments = 48, int tubeSegments = 10);

	struct CameraHelperParams {
		raylib::Vector3 position;
		raylib::Vector3 target;
		raylib::Vector3 up{ 0.0f, 1.0f, 0.0f };
		float fovy{ 45.0f };                
		float near{ 0.1f };
		float far{ 1000.0f };
		int   projection{ ::CAMERA_PERSPECTIVE };
	};

	CameraHelperCommand BuildCameraHelperCommand(const CameraHelperParams& p, float helper_size = 2.0f);

} // namespace Long

#endif // !_DRAW_DEBUG_HELPER_HPP_