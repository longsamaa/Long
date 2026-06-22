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