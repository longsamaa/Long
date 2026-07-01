#ifndef _RENDER_SYSTEM_HPP_
#define _RENDER_SYSTEM_HPP_
#include <entt/entt.hpp>
#include <vector>
#include "system/RenderStats.hpp"
#include "engine/render/CommandQueue.hpp"
namespace Long {
	class AssetManager;
	// Builds the draw command list from a PRE-CULLED list of visible entities. The
	// visibility decision lives in IVisibility (see VisibilitySystem) so the culling
	// strategy (linear scan today, quadtree later) can change without touching this.
	void RenderSystem(entt::registry& registry, AssetManager& assets, CommandQueue& queue,
		const std::vector<entt::entity>& visible, RenderStats& stats);
}
#endif // !_RENDER_SYSTEM_HPP_