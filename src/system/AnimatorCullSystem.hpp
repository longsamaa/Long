#pragma once
#ifndef _ANIMATOR_CULL_SYSTEM_HPP_
#define _ANIMATOR_CULL_SYSTEM_HPP_
#include <entt/entt.hpp>
namespace Long {
	class FrustumCulling;
	// Decides Animator::isVisible for this frame: an animated model counts as
	// visible when ANY mesh AABB under its root is inside the CAMERA frustum.
	// Separate from gatherVisible on purpose -- that one also runs for shadow
	// passes with light frustums, which cover far more than the screen and
	// would keep every animator "visible" under a scene-wide directional light.
	// Runs against the camera frustum only, once per frame (ScenePreparePass).
	void AnimatorCullSystem(entt::registry& registry, const FrustumCulling* frustum);
}
#endif // !_ANIMATOR_CULL_SYSTEM_HPP_
