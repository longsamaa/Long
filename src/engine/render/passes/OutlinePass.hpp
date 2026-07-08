#pragma once
#ifndef _OUTLINE_PASS_HPP_
#define _OUTLINE_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
#include "engine/render/GLRenderTarget.hpp"
#include "raylib-cpp.hpp"
namespace Long {
	// Edge-detects ctx.maskTarget and draws the outline over the screen. Runs
	// AFTER CompositePass (the scene is already on screen).
	//
	// The edge detection is SEPARABLE: instead of one shader sampling a
	// (2*size+1)^2 box per pixel (O(size^2)), it runs a 1D horizontal pass then a
	// 1D vertical pass (O(size) each) -- e.g. size=5 goes from 121 to 22 fetches per
	// pixel, which is what stops the fullscreen lag when zoomed in.
	class OutlinePass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;

		int outlineSize = 5;                               // thickness in mask texels
		raylib::Color outlineColor = { 255, 160, 0, 255 }; // orange
		// Run the dilation passes at 1/downscale resolution (1 = full res, 2 = half,
		// 4 = quarter). Bigger = cheaper fill but a softer/less precise outline.
		int downscale = 1;
	private:
		uint32_t m_shaderId = UINT32_MAX;                  // resolved lazily
		GLRenderTarget m_tempTarget;  // holds the horizontal-pass result (the dilated mask)
	};
}
#endif // !_OUTLINE_PASS_HPP_