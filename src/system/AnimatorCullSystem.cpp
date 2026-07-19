#include "AnimatorCullSystem.hpp"
#include "core/Components.hpp"
#include "engine/visibility/FrustumCulling.hpp"

namespace Long {
	// The Animator sits on the model's root entity, which has no mesh/AABB of
	// its own; the drawable AABBs live on the child (mesh, material) entities.
	// Visible = any AABB in the subtree passes the frustum test.
	static bool AnySubtreeAABBVisible(entt::registry& registry, entt::entity e,
		const FrustumCulling& frustum)
	{
		if (const WorldAABB* aabb = registry.try_get<WorldAABB>(e)) {
			if (frustum.isVisible(aabb->min, aabb->max)) {
				return true;
			}
		}
		if (const Hierarchy* h = registry.try_get<Hierarchy>(e)) {
			for (entt::entity child : h->children) {
				if (child != entt::null && registry.valid(child)
					&& AnySubtreeAABBVisible(registry, child, frustum)) {
					return true;
				}
			}
		}
		return false;
	}

	void AnimatorCullSystem(entt::registry& registry, const FrustumCulling* frustum) {
		auto view = registry.view<Animator>();
		for (entt::entity e : view) {
			Animator& anim = view.get<Animator>(e);
			if (anim.culling_mode == Animator::CullingMode::Always || !frustum) {
				anim.isVisible = true;
				continue;
			}
			anim.isVisible = AnySubtreeAABBVisible(registry, e, *frustum);
		}
	}
}
