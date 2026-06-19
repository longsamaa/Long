#pragma once
#ifndef _RAYCAST_SYSTEM_HPP_
#define _RAYCAST_SYSTEM_HPP_
#include <entt/entt.hpp>
#include <raylib-cpp.hpp>
namespace Long {
	// Result of casting a ray against the scene's BoxCollider3D entities.
	struct RaycastHit {
		bool hit = false;
		entt::entity entity = entt::null;
		float distance = 0.0f;           // along the ray to the hit point
		raylib::Vector3 point = { 0,0,0 }; // world-space hit position
	};

	// Cast `ray` against every entity that has BoxCollider3D + WorldTransform.
	// Returns the CLOSEST hit (smallest distance), or { hit=false } if none.
	RaycastHit RaycastSystem(entt::registry& registry, const raylib::Ray& ray);
}
#endif // !_RAYCAST_SYSTEM_HPP_