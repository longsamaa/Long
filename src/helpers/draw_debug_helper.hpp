#pragma once
#ifndef _DRAW_DEBUG_HELPER_HPP_
#define _DRAW_DEBUG_HELPER_HPP_

#include "raylib-cpp.hpp"

namespace Long {
	// Transform a LOCAL axis-aligned box by a world matrix and return the
	// enclosing WORLD-space AABB. Use it to get a BoundingBox you can pass to
	// DrawBoundingBox() for debug drawing (e.g. an entity's collider).
	//
	// For rotated boxes this is a conservative bound (the enclosing AABB), which
	// is what raylib's DrawBoundingBox expects anyway.
	raylib::BoundingBox MakeWorldBoundingBox(const raylib::BoundingBox& localBox,
		const raylib::Matrix& world);

	// Draw a circle of `radius` around `center`, lying on the plane spanned by the
	// unit vectors `u` and `v` (e.g. a rotation ring on its axis plane). Drawn as
	// `segments` line segments. Call inside the camera's 3D mode.
	void DrawCircle3D(raylib::Vector3 center, raylib::Vector3 u, raylib::Vector3 v,
		float radius, raylib::Color color, float thickness = 1.0f,
		int segments = 48);

	// Draw a flat ring (annulus) of mean `radius` and visual width `width`, on the
	// plane spanned by `u`/`v`. Unlike line width, this is real geometry so it can
	// be as thick as you like and looks consistent across GPUs. 3D-mode only.
	void DrawRing3D(raylib::Vector3 center, raylib::Vector3 u, raylib::Vector3 v,
		float radius, float width, raylib::Color color, int segments = 48);

	// Draw a torus (a solid "tube" ring): a small circle of radius `tube` swept
	// around the main circle of `radius`, on the plane spanned by `u`/`v` (normal
	// n = u x v). Looks round from any angle. 3D-mode only.
	void DrawTorus3D(raylib::Vector3 center, raylib::Vector3 u, raylib::Vector3 v,
		float radius, float tube, raylib::Color color,
		int ringSegments = 48, int tubeSegments = 10);
} // namespace Long

#endif // !_DRAW_DEBUG_HELPER_HPP_