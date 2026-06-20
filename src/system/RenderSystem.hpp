#ifndef _RENDER_SYSTEM_HPP_
#define _RENDER_SYSTEM_HPP_
#include <entt/entt.hpp>
#include "system/RenderStats.hpp"
#include "../engine/render/CommandQueue.hpp"
namespace Long {
	class AssetManager;
	void RenderSystem(entt::registry& registry, AssetManager& assets, CommandQueue& queue);
}
#endif // !_RENDER_SYSTEM_HPP_