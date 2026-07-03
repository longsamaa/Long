#pragma once
#ifndef _SCENE_PASS_HPP_
#define _SCENE_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
namespace Long {
	// Draws the prepared batches (see ScenePreparePass) from the camera view into
	// the HDR scene target, plus the skybox.
	class ScenePass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	};
}
#endif // !_SCENE_PASS_HPP_
