#pragma once
#ifndef _WORLD_BOUND_SYSTEM_HPP_
#define _WORLD_BOUND_SYSTEM_HPP_
#include <entt/entt.hpp>
#include "engine/AssetManager.hpp"
namespace Long {
	//World aabb update 
	void WorldBoundsSystem(entt::registry& registry, AssetManager& asset_manager);
}
#endif // !_WORLD_BOUND_SYSTEM_HPP_
