#pragma once
#ifndef _BRIGHT_PASS_HPP_
#define _BRIGHT_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
namespace Long {
	//Bright pass
	class BrightPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
		// Only true HDR pixels bloom: the knee band starts at threshold*(1-knee)
		// = 1.05, just above the LDR ceiling (1.0), so normally-lit surfaces
		// (diffuse+ambient <= 1) never glow -- only emissives (intensity ~5) and
		// hot specular highlights do.
		float u_threshold{ 1.5f };
		float u_softKnee{ 0.3f };
	public: 
		uint32_t down_scale{ 1 };
	private:
		uint32_t m_shaderId{ UINT32_MAX };
	};
}
#endif // !_BRIGHT_PASS_HPP_