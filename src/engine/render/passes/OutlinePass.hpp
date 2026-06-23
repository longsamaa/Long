#pragma once
#ifndef _OUTLINE_PASS_HPP_
#define _OUTLINE_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
#include "raylib-cpp.hpp"
namespace Long {
	// Edge-detects ctx.maskTarget and draws the outline over the screen. Runs
	// AFTER CompositePass (the scene is already on screen). The outline shader
	// colors only the pixels just outside the selection silhouette.
	class OutlinePass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;

		int outlineSize = 5;                               // thickness in mask texels
		raylib::Color outlineColor = { 255, 160, 0, 255 }; // orange
	private:
		uint32_t m_shaderId = UINT32_MAX;                  // resolved lazily
	};
}
#endif // !_OUTLINE_PASS_HPP_