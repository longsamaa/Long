#pragma once
#ifndef _COMPOSITE_PASS_HPP_
#define _COMPOSITE_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
namespace Long {

	// Final pass: blit the scene target to the screen (the viewport). Must run
	// after ScenePass. Draws the render texture flipped (GL Y is inverted).
	class CompositePass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	};

}
#endif // !_COMPOSITE_PASS_HPP_
