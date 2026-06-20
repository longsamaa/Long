#ifndef _RENDER_SYSTEM_HPP_
#define _RENDER_SYSTEM_HPP_
#include <entt/entt.hpp>
#include "system/RenderStats.hpp"
#include "../engine/render/CommandQueue.hpp"
namespace Long {
	class AssetManager;

	// Draws every entity that has WorldTransform + MeshFilter + MeshRenderer,
	// using the mesh + material they reference (looked up in AssetManager).
	// Run TransformSystem first. Call between camera Begin3D()/End3D().
	//
	// Fills `stats` with this frame's draw-call / triangle counts.
	void RenderSystem(entt::registry& registry, AssetManager& assets, CommandQueue& queue);
}
#endif // !_RENDER_SYSTEM_HPP_
