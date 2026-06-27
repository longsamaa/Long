#pragma once
#ifndef _FXAA_PASS_HPP_
#define _FXAA_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
namespace Long {

	// Final pass: read the composited finalTarget, run FXAA, and blit the
	// anti-aliased result to the screen. Cheaper than MSAA -- good for weak GPUs.
	class FXAAPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	private:
		uint32_t m_shaderId{ UINT32_MAX }; // resolved lazily
	};

}
#endif // !_FXAA_PASS_HPP_
