#include "engine/render/passes/ShadowPass.hpp"
#include "engine/AssetManager.hpp"
#include "engine/render/RenderState.hpp"
#include "system/RenderSystem.hpp"
#include "engine/Logger.hpp"
#include "helpers/TimerHelper.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <format>

namespace Long {
	void ShadowPass::execute(RenderContext& ctx) {
		if (!ctx.assets || !ctx.registry || !ctx.lights || !ctx.shadow_depth_target) {
			return;
		}
		ctx.shadow_depth_target->SetFormat(RenderTarget::Format::DEPTH);
		ctx.shadow_depth_target->Resize(m_resolution, m_resolution);
		ctx.lights->shadowsEnabled = false;
		ctx.lights->shadowMapTexId = 0;

		const LightParameter* dirLight = nullptr;
		for (uint32_t i = 0; i < ctx.lights->size; ++i) {
			const LightParameter& l = ctx.lights->lights[i];
			if (l.type == (uint32_t)LightType::Directional) {
				dirLight = &l;
				break;
			}
		}
		if (!dirLight) {
			return; 
		}

		uint32_t depthShaderId = ctx.assets->GetShaderId("shadow_depth");
		if (!ctx.assets->IsValidShader(depthShaderId)) {
			return;
		}
		if (ctx.shadow_depth_target->DepthTextureId() == 0) {
			return; 
		}
		raylib::Vector3 dir = raylib::Vector3(dirLight->direction).Normalize();
		if (dir.LengthSqr() < 1e-6f) {
			dir = { 0.0f, -1.0f, 0.0f };
		}
		raylib::Vector3 target{ 0.0f, 0.0f, 0.0f };
		raylib::Vector3 eye = target.Subtract(dir.Scale(m_distance));
		raylib::Vector3 up = (std::fabs(dir.y) > 0.99f)
			? raylib::Vector3{ 0.0f, 0.0f, 1.0f }
			: raylib::Vector3{ 0.0f, 1.0f, 0.0f };
		::Matrix view = MatrixLookAt(eye, target, up);
		::Matrix proj = MatrixOrtho(-m_orthoExtent, m_orthoExtent,
			-m_orthoExtent, m_orthoExtent,
			0.1f, m_distance * 2.0f);
		raylib::Matrix lightViewProj = MatrixMultiply(view, proj);
		m_lightFrustum.buildFromMatrix(lightViewProj);
		m_queue.Clear();
		m_visibility->gatherVisible(*ctx.registry, &m_lightFrustum, m_visible,
			ctx.renderStats.culledEntities);
		RenderSystem(*ctx.registry, *ctx.assets, m_queue, m_visible, ctx.renderStats);
		m_queue.Sort();
		m_queue.BuildBatches();
		{
			ScopedDepthTest depth(true);
			ScopedDepthMask mask(true);
			ScopedBackfaceCull cull(true);
			ctx.shadow_depth_target->Bind();
			raylib::Color::Black().ClearBackground(); // clears depth to 1.0 too
			m_queue.ExecuteDepth(*ctx.assets, lightViewProj, depthShaderId);
			ctx.shadow_depth_target->Unbind();
		}
		ctx.lights->lightViewProj = lightViewProj;
		ctx.lights->shadowMapTexId = ctx.shadow_depth_target->DepthTextureId();
		ctx.lights->shadowMapSize = ctx.shadow_depth_target->Width();
		ctx.lights->shadowsEnabled = true;
	}
}
