#include "RayCastSystem.hpp"
#include "core/Components.hpp"

namespace Long {
	// Transform a local AABB by a world matrix and return the enclosing
	// world-space AABB. (For rotated boxes this is a conservative bound -- fine
	// for picking; tiles are usually axis-aligned anyway.)
	static raylib::BoundingBox WorldAABB(const BoxCollider3D& collider, const Matrix& world) {
		const Vector3& mnL = collider.box.min;
		const Vector3& mxL = collider.box.max;
		// All 8 corners of the local box.
		Vector3 corners[8] = {
			{mnL.x, mnL.y, mnL.z},
			{mxL.x, mnL.y, mnL.z},
			{mnL.x, mxL.y, mnL.z},
			{mnL.x, mnL.y, mxL.z},
			{mxL.x, mxL.y, mnL.z},
			{mxL.x, mnL.y, mxL.z},
			{mnL.x, mxL.y, mxL.z},
			{mxL.x, mxL.y, mxL.z},
		};

		Vector3 mn = Vector3Transform(corners[0], world);
		Vector3 mx = mn;
		for (int i = 1; i < 8; ++i) {
			Vector3 w = Vector3Transform(corners[i], world);
			mn = Vector3Min(mn, w);
			mx = Vector3Max(mx, w);
		}
		return raylib::BoundingBox(mn, mx);
	}

	RaycastHit RaycastSystem(entt::registry& registry, const raylib::Ray& ray) {
		RaycastHit best;
		float bestDist = 0.0f;

		auto view = registry.view<BoxCollider3D, WorldTransform>();
		for (entt::entity e : view) {
			const auto& [box, wt] = view.get<BoxCollider3D, WorldTransform>(e);
			raylib::BoundingBox aabb = WorldAABB(box, wt.matrix);
			RayCollision col = GetRayCollisionBox(ray, aabb);
			if (col.hit && (!best.hit || col.distance < bestDist)) {
				best.hit = true;
				best.entity = e;
				best.distance = col.distance;
				best.point = col.point;
				bestDist = col.distance;
			}
		}
		return best;
	}
}