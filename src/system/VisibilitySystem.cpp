#include "VisibilitySystem.hpp"
#include "core/Components.hpp"
#include "engine/visibility/FrustumCulling.hpp"

namespace Long {
	void LinearVisibility::gatherVisible(entt::registry& registry,
		const FrustumCulling* frustum,
		std::vector<entt::entity>& out,
		uint32_t& outCulledCount)
	{
		out.clear();
		outCulledCount = 0;
		auto view = registry.view<MatrixTransform, MeshFilter, MeshRenderer>();
		out.reserve(view.size_hint());
		for (auto e : view) {
			if (frustum) {
				if (const WorldAABB* aabb = registry.try_get<WorldAABB>(e)) {
					if (!frustum->isVisible(aabb->min, aabb->max)) {
						++outCulledCount;
						continue;
					}
				}
			}
			out.push_back(e);
		}
	}
}