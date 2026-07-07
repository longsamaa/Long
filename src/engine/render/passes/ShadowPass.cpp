#include "engine/render/passes/ShadowPass.hpp"
#include "engine/AssetManager.hpp"
#include "engine/render/RenderState.hpp"
#include "system/RenderSystem.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

namespace Long {
	bool ShadowPass::buildLightMatrix(const LightParameter& light, raylib::Matrix& out, const float& range) const {
		raylib::Vector3 dir = raylib::Vector3(light.direction).Normalize();
		if (dir.LengthSqr() < 1e-6f) {
			dir = { 0.0f, -1.0f, 0.0f };
		}
		raylib::Vector3 up = (std::fabs(dir.y) > 0.99f)
			? raylib::Vector3{ 0.0f, 0.0f, 1.0f }
		: raylib::Vector3{ 0.0f, 1.0f, 0.0f };

		if (light.type == (uint32_t)LightType::Directional) {
			raylib::Vector3 target{ 0.0f, 0.0f, 0.0f };
			raylib::Vector3 eye = target.Subtract(dir.Scale(m_distance));
			raylib::Matrix view = raylib::Matrix(MatrixLookAt(eye, target, up));
			raylib::Matrix proj = raylib::Matrix(MatrixOrtho(-m_orthoExtent, m_orthoExtent,
				-m_orthoExtent, m_orthoExtent,
				0.1f, m_distance * 2.0f));
			out = view.Multiply(proj);
			return true;
		} else if (light.type == (uint32_t)LightType::Spot) {
			float fovy = 2.0f * std::acos(std::fmin(std::fmax(light.outerCos, -1.0f), 1.0f));
			raylib::Vector3 eye = light.position;
			raylib::Matrix view = raylib::Matrix(MatrixLookAt(eye, eye.Add(dir), up));
			raylib::Matrix proj = raylib::Matrix(MatrixPerspective(fovy, 1.0, m_near, (double)range));
			out = view.Multiply(proj);
			return true;
		}
		else if (light.type == (uint32_t)LightType::Point) {
			raylib::Vector3 eye = light.position; 
			const Vector3 directions[6] = {
				{  1,  0,  0 }, // +X
				{ -1,  0,  0 }, // -X
				{  0,  1,  0 }, // +Y
				{  0, -1,  0 }, // -Y
				{  0,  0,  1 }, // +Z
				{  0,  0, -1 }, // -Z
			};

			const Vector3 ups[6] = {
				{  0, -1,  0 }, // +X
				{  0, -1,  0 }, // -X
				{  0,  0,  1 }, // +Y
				{  0,  0, -1 }, // -Y
				{  0, -1,  0 }, // +Z
				{  0, -1,  0 }, // -Z
			};

			for (int i = 0; i < 6; i++)
			{
				raylib::Matrix view = raylib::Matrix(MatrixLookAt(
					eye,
					Vector3Add(eye, directions[i]),
					ups[i]
				));
				raylib::Matrix proj = raylib::Matrix(MatrixPerspective(90 * DEG2RAD, m_near, 0.1, (double)range));
				raylib::Matrix lightProjMatrix = view.Multiply(proj); 
			}

		}
		return false; // point lights need a cubemap -- not supported
	}

	void ShadowPass::execute(RenderContext& ctx) {
		if (!ctx.assets || !ctx.registry || !ctx.lights) {
			return;
		}
		SceneLights& lights = *ctx.lights;
		lights.shadowCount = 0;

		uint32_t depthShaderId = ctx.assets->GetShaderId("shadow_depth");
		if (!ctx.assets->IsValidShader(depthShaderId)) {
			return;
		}

		for (uint32_t i = 0; i < lights.size; ++i) {
			LightParameter& light = lights.lights[i];
			light.shadowIndex = -1;
			if (!light.castsShadows) {
				continue;
			}
			if (lights.shadowCount >= (uint32_t)SceneLights::kMaxShadows) {
				break;
			}

			raylib::Matrix lightViewProj;
			const float& range = light.range; 
			if (!buildLightMatrix(light, lightViewProj,range)) {
				continue;
			}

			uint32_t slot = lights.shadowCount;
			RenderTarget& target = m_targets[slot];
			target.SetFormat(RenderTarget::Format::DEPTH);
			target.Resize(m_resolution, m_resolution);
			if (target.DepthTextureId() == 0) {
				continue; // depth framebuffer failed to allocate
			}

			// Cull with THIS light's frustum and build private batches.
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
				target.Bind();
				raylib::Color::Black().ClearBackground(); // clears depth to 1.0 too
				m_queue.ExecuteDepth(*ctx.assets, lightViewProj, depthShaderId);
				target.Unbind();
			}

			lights.shadows[slot].lightViewProj = lightViewProj;
			lights.shadows[slot].depthTexId = target.DepthTextureId();
			light.shadowIndex = (int)slot;
			lights.shadowCount++;
		}
	}
}