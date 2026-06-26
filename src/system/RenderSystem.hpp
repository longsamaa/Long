#ifndef _RENDER_SYSTEM_HPP_
#define _RENDER_SYSTEM_HPP_
#include <entt/entt.hpp>
#include "system/RenderStats.hpp"
#include "engine/render/CommandQueue.hpp"
namespace Long {
	class AssetManager;
	class FrustumCulling;
	// Builds the draw command list from the ECS view. When `frustum` is non-null,
	// entities whose world AABB lies outside the view are skipped and counted in
	// stats.culledEntities.
	void RenderSystem(entt::registry& registry, AssetManager& assets, CommandQueue& queue,
		const FrustumCulling* frustum, RenderStats& stats);
}
#endif // !_RENDER_SYSTEM_HPP_