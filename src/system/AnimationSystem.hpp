#pragma once
#ifndef _ANIMATION_SYSTEM_HPP_
#define _ANIMATION_SYSTEM_HPP_
#include <entt/entt.hpp>
namespace Long {
	void AnimationSystem(entt::registry& registry, float dt);
	static void Animation_FindKeys(const std::vector<float>& times, float time,
		size_t& i0, size_t& i1, float& f); 
}
#endif // !_ANIMATION_SYSTEM_HPP_
