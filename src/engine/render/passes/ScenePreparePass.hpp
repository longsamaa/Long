#pragma once
#ifndef _SCENE_PREPARE_PASS_HPP_
#define _SCENE_PREPARE_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
#include "system/VisibilitySystem.hpp"
#include <memory>
#include <vector>
namespace Long {
	// Builds the frame's draw batches ONCE, up front, so later passes can reuse
	// them: gather visible entities (camera frustum) -> RenderSystem submits draw
	// commands -> Sort -> BuildBatches. ShadowPass draws these same batches from
	// the light's view (depth), ScenePass draws them from the camera (color).
	class ScenePreparePass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	private:
		std::unique_ptr<IVisibility> m_visibility{ std::make_unique<LinearVisibility>() };
		std::vector<entt::entity> m_visible;
	};
}
#endif // !_SCENE_PREPARE_PASS_HPP_
