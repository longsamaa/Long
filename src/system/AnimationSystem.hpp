#pragma once
#ifndef _ANIMATION_SYSTEM_HPP_
#define _ANIMATION_SYSTEM_HPP_
#include <entt/entt.hpp>
namespace Long {
	// Advances every AnimationPlayer and writes the sampled T/R/S keys into the
	// target entities' Transforms via registry.patch<Transform>, so the Scene's
	// on_update observer bumps version + DirtyTransform and TransformSystem picks
	// the change up. Run BEFORE TransformSystem (which feeds SkinningSystem).
	void AnimationSystem(entt::registry& registry, float dt);
}
#endif // !_ANIMATION_SYSTEM_HPP_
