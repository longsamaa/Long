#include "helpers/draw_debug_helper.hpp"
#include "raymath.h"

namespace Long {

	raylib::BoundingBox MakeWorldBoundingBox(const raylib::BoundingBox& localBox,
											 const raylib::Matrix& world) {
		const Vector3& mn = localBox.min;
		const Vector3& mx = localBox.max;

		// The 8 corners of the local box.
		Vector3 corners[8] = {
			{ mn.x, mn.y, mn.z },
			{ mx.x, mn.y, mn.z },
			{ mn.x, mx.y, mn.z },
			{ mn.x, mn.y, mx.z },
			{ mx.x, mx.y, mn.z },
			{ mx.x, mn.y, mx.z },
			{ mn.x, mx.y, mx.z },
			{ mx.x, mx.y, mx.z },
		};

		// Transform every corner to world space and take the enclosing AABB.
		Vector3 worldMin = Vector3Transform(corners[0], world);
		Vector3 worldMax = worldMin;
		for (int i = 1; i < 8; ++i) {
			Vector3 w = Vector3Transform(corners[i], world);
			worldMin = Vector3Min(worldMin, w);
			worldMax = Vector3Max(worldMax, w);
		}

		return raylib::BoundingBox(worldMin, worldMax);
	}

	void DrawCircle3D(raylib::Vector3 center, raylib::Vector3 u, raylib::Vector3 v,
					  float radius, raylib::Color color, int segments) {
		// Step around the circle: p(a) = center + cos(a)*u*r + sin(a)*v*r,
		// connecting consecutive points with line segments.
		raylib::Vector3 prev = center.Add(u.Scale(radius));
		for (int s = 1; s <= segments; ++s) {
			float a = (float)s / segments * 2.0f * PI;
			raylib::Vector3 p = center
				.Add(u.Scale(cosf(a) * radius))
				.Add(v.Scale(sinf(a) * radius));
			p.DrawLine3D(prev, color);
			prev = p;
		}
	}

} // namespace Long
