#include "RayCastSystem.hpp"
#include "core/Components.hpp"
#include "helpers/draw_debug_helper.hpp"

namespace Long {

	RaycastHit RaycastSystem(entt::registry& registry, const raylib::Ray& ray) {
		RaycastHit best;
		float bestDist = 0.0f;

		auto view = registry.view<BoxCollider3D, WorldTransform>();
		for (entt::entity e : view) {
			const auto& [box, wt] = view.get<BoxCollider3D, WorldTransform>(e);
			raylib::BoundingBox aabb = MakeWorldBoundingBox(box.box, wt.matrix);
			RayCollision col = raylib::Ray(ray).GetCollision(aabb);
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