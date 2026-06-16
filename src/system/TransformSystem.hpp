#pragma once
#ifndef _TRANSFORM_SYSTEM_HPP_
#define _TRANSFORM_SYSTEM_HPP_
#include <entt/entt.hpp>
namespace Long {
	// Computes WorldTransform for every entity from its local Transform and its
	// parent's WorldTransform (world = parent.world * local). Walks each root
	// (an entity with no parent / no Hierarchy) down through its children.
	//
	// Run this every frame BEFORE RenderSystem.
	void TransformSystem(entt::registry& registry);
}
#endif // !_TRANSFORM_SYSTEM_HPP_
