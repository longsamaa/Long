#include "RayCastSystem.hpp"
#include "core/Components.hpp"
#include "helpers/draw_debug_helper.hpp"

namespace Long {
	RaycastHit RaycastSystem(entt::registry& registry, const raylib::Ray& ray) {
		RaycastHit best;
		float bestDist = 0.0f;
		auto view = registry.view<WorldAABB>();
		for (entt::entity e : view) {
			const auto& world_aabb = view.get<WorldAABB>(e);
			RayCollision col = raylib::Ray(ray).GetCollision({world_aabb.min,world_aabb.max});
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