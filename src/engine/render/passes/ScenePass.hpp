#pragma once
#ifndef _SCENE_PASS_HPP_
#define _SCENE_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
#include "system/VisibilitySystem.hpp"
#include <memory>
#include <vector>
namespace Long {
	class ScenePass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	private:
		std::unique_ptr<IVisibility> m_visibility{ std::make_unique<LinearVisibility>() };
		std::vector<entt::entity> m_visible;
	};
}
#endif // !_SCENE_PASS_HPP_