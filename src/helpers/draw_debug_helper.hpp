#pragma once
#ifndef _DRAW_DEBUG_HELPER_HPP_
#define _DRAW_DEBUG_HELPER_HPP_

#include "raylib-cpp.hpp"
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

	CameraHelperCommand BuildCameraHelperCommand(const raylib::Camera3D& camera, float nearDist =5.0f); 

} // namespace Long

#endif // !_DRAW_DEBUG_HELPER_HPP_