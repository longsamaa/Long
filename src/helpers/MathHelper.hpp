#pragma once
#ifndef _MATH_HELPER_HPP_
#define _MATH_HELPER_HPP_

#include "raylib-cpp.hpp"

namespace Long {
	//Math + Vector Helper for raylib
	static float ClosestAxisParam(const raylib::Ray& ray, raylib::Vector3 origin, raylib::Vector3 dir) {
		raylib::Vector3 rdir = ray.direction;
		raylib::Vector3 w0 = raylib::Vector3(ray.position).Subtract(origin);
		float a = rdir.DotProduct(rdir);
		float b = rdir.DotProduct(dir);
		float c = dir.DotProduct(dir);
		float d = rdir.DotProduct(w0);
		float e = dir.DotProduct(w0);
		float denom = a * c - b * b;
		if (fabsf(denom) < 1e-6f) return 0.0f;
		return (a * e - b * d) / denom;
	}

	// Shortest distance between the mouse ray and the segment [origin, origin+dir*len]
	// (dir unit length). Used to pick a tilted axis handle without an AABB, which
	// would balloon when the axis isn't world-aligned. `outT` returns the clamped
	// parameter along the axis (0..len) of the closest point.
	static float RayAxisDistance(const raylib::Ray& ray, raylib::Vector3 origin,
		raylib::Vector3 dir, float len, float& outT) {
		float t = ClosestAxisParam(ray, origin, dir);   // param along the axis line
		t = (t < 0.0f) ? 0.0f : (t > len ? len : t);    // clamp to the segment
		outT = t;
		raylib::Vector3 onAxis = origin.Add(dir.Scale(t));
		// Closest point on the ray to that axis point, then measure the gap.
		raylib::Vector3 w = onAxis.Subtract(ray.position);
		float s = w.DotProduct(ray.direction);          // ray.direction is unit
		if (s < 0.0f) s = 0.0f;                          // ray starts at camera
		raylib::Vector3 onRay = raylib::Vector3(ray.position).Add(raylib::Vector3(ray.direction).Scale(s));
		return onAxis.Distance(onRay);
	}

	static raylib::Vector3 RayPlane(const raylib::Ray& ray, raylib::Vector3 p, raylib::Vector3 n, bool& ok) {
		raylib::Vector3 rdir = ray.direction;
		float denom = n.DotProduct(rdir);
		if (fabsf(denom) < 1e-6f) { ok = false; return p; }
		float t = p.Subtract(ray.position).DotProduct(n) / denom;
		ok = (t > 0.0f);
		return raylib::Vector3(ray.position).Add(rdir.Scale(t));
	}

	// Two orthonormal vectors spanning the plane with normal `n` (unit length).
	static void PlaneBasis(const raylib::Vector3& n, raylib::Vector3& u, raylib::Vector3& v) {
		// Pick a reference axis that isn't parallel to n.
		raylib::Vector3 ref = (fabsf(n.x) < 0.9f) ? raylib::Vector3{ 1, 0, 0 }
		: raylib::Vector3{ 0, 1, 0 };
		u = n.CrossProduct(ref).Normalize();
		v = n.CrossProduct(u).Normalize();
	}

	// Angle (radians) of `point` around `center` on the plane (u,v).
	static float RingAngle(raylib::Vector3 point, raylib::Vector3 center,
		raylib::Vector3 u, raylib::Vector3 v) {
		raylib::Vector3 d = point.Subtract(center);
		return atan2f(d.DotProduct(v), d.DotProduct(u));
	}
} // namespace Long

#endif // !_MATH_HELPER_HPP_