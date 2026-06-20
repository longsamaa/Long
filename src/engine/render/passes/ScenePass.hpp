#pragma once
#ifndef _SCENE_PASS_HPP_
#define _SCENE_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
namespace Long {
	// First pass: render the 3D ECS scene into ctx.sceneTarget. Fills
	// ctx.renderStats. Later passes (outline, composite) read the target.
	class ScenePass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	};
}
#endif // !_SCENE_PASS_HPP_