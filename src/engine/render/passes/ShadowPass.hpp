#pragma once
#ifndef _SHADOW_PASS_HPP_
#define _SHADOW_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
#include "engine/render/CommandQueue.hpp"
#include "engine/visibility/FrustumCulling.hpp"
#include "system/VisibilitySystem.hpp"
#include <memory>
#include <vector>
namespace Long {
	class ShadowPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
		void SetOrthoExtent(float extent) { m_orthoExtent = extent; }
		void SetResolution(uint32_t res) { m_resolution = res; }

	private:
		FrustumCulling m_lightFrustum;
		std::unique_ptr<IVisibility> m_visibility{ std::make_unique<LinearVisibility>() };
		std::vector<entt::entity> m_visible;
		CommandQueue m_queue;

		uint32_t m_resolution{ 2048 };
		float m_orthoExtent{ 110.0f }; // half-size of covered area (ground is +/-100)
		float m_distance{ 150.0f };    // how far back the light "camera" sits
	};
}
#endif // !_SHADOW_PASS_HPP_
