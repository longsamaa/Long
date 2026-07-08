#pragma once
#ifndef _SHADOW_PASS_HPP_
#define _SHADOW_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
#include "engine/render/CommandQueue.hpp"
#include "engine/render/GLRenderTarget.hpp"
#include "engine/visibility/FrustumCulling.hpp"
#include "system/VisibilitySystem.hpp"
#include "system/LightSystem.hpp"
#include <array>
#include <memory>
#include <vector>
namespace Long {
	class ShadowPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;

		void SetOrthoExtent(float extent) { m_orthoExtent = extent; }
		void SetResolution(uint32_t res) { m_resolution = res; }

	private:
		bool buildLightMatrix(const LightParameter& light, raylib::Matrix& out, const float& range) const;
		// Render a point light's 6-face depth cube. Returns false if it couldn't.
		bool renderPointCube(RenderContext& ctx, const LightParameter& light,
			const uint32_t& slot, uint32_t pointDepthShaderId);
		// Render a directional light.(directional light, spot)
		bool renderDirectionLightDepth(RenderContext& ctx, const uint32_t& slot, const LightParameter& light);


		std::array<GLRenderTarget, SceneLights::kMaxShadows> m_targets;
		std::array<GLRenderTarget, SceneLights::kMaxCubeShadows> m_cubeTargets;

		//Rect light target 
		FrustumCulling m_lightFrustum;
		std::unique_ptr<IVisibility> m_visibility{ std::make_unique<LinearVisibility>() };
		std::vector<entt::entity> m_visible;
		CommandQueue m_queue;

		uint32_t m_resolution{ 2048 };
		uint32_t m_cubeResolution{ 1024 };
		float m_orthoExtent{ 110.0f };
		float m_near{ 0.1f }; 
		float m_distance{ 1000.0f };
		float m_spotFar{ 200.0f };
	};
}
#endif // !_SHADOW_PASS_HPP_