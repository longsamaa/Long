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
					  float radius, raylib::Color color, int segments = 48);

} // namespace Long

#endif // !_DRAW_DEBUG_HELPER_HPP_
