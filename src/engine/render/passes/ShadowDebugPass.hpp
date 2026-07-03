#pragma once
#ifndef _SHADOW_DEBUG_PASS_HPP_
#define _SHADOW_DEBUG_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
namespace Long {
	// DEBUG overlay: blits the shadow depth map into a corner of the screen so we
	// can see whether the depth pass actually wrote occluders. Remove/disable once
	// shadows are verified.
	class ShadowDebugPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	};
}
#endif // !_SHADOW_DEBUG_PASS_HPP_
