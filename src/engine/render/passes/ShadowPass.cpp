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
		}
		else if (light.type == (uint32_t)LightType::Spot) {
			float fovy = 2.0f * std::acos(std::fmin(std::fmax(light.outerCos, -1.0f), 1.0f));
			raylib::Vector3 eye = light.position;
			raylib::Matrix view = raylib::Matrix(MatrixLookAt(eye, eye.Add(dir), up));
			raylib::Matrix proj = raylib::Matrix(MatrixPerspective(fovy, 1.0, m_near, (double)range));
			out = view.Multiply(proj);
			return true;
		}
		return false; // point lights need a cubemap -- not supported
	}

	void ShadowPass::execute(RenderContext& ctx) {
		if (!ctx.assets || !ctx.registry || !ctx.lights) {
			return;
		}
		SceneLights& lights = *ctx.lights;
		lights.shadowCount = 0;
		uint32_t pointDepthShaderId = ctx.assets->GetShaderId("shadow_depth_point");
		uint32_t depthShaderId = ctx.assets->GetShaderId("shadow_depth");
		lights.cubeShadowCount = 0;
		for (uint32_t i = 0; i < lights.size; ++i) {
			LightParameter& light = lights.lights[i];
			light.shadowIndex = -1;
			light.cubeShadowIndex = -1;
			if (!light.castsShadows) {
				continue;
			}
			const float& range = light.range;
			// ---- Point light: omnidirectional depth cube ----
			if (light.type == (uint32_t)LightType::Point) {
				if (lights.cubeShadowCount >= (uint32_t)SceneLights::kMaxCubeShadows) {
					continue;
				}
				uint32_t cubeSlot = lights.cubeShadowCount;
				if (renderPointCube(ctx, light, cubeSlot, pointDepthShaderId)) {
					lights.cubeShadows[cubeSlot].cubeTexId =
						m_cubeTargets[cubeSlot].Depth().id;
					lights.cubeShadows[cubeSlot].lightPos = light.position;
					lights.cubeShadows[cubeSlot].range = range;
					light.cubeShadowIndex = (int)cubeSlot;
					lights.cubeShadowCount++;
				}
				continue;
			}

			// ---- Directional / Spot: single 2D depth map ----

			//Render direction light
			uint32_t slot = lights.shadowCount;
			renderDirectionLightDepth(ctx, lights, slot, light, range, depthShaderId);
		}
	}

	bool ShadowPass::renderPointCube(RenderContext& ctx, const LightParameter& light,
		const uint32_t& slot, uint32_t pointDepthShaderId)
	{
		if (!ctx.assets->IsValidShader(pointDepthShaderId)) {
			return false;
		}
		GLRenderTarget& cube = m_cubeTargets[slot];
		cube.SetFormat(GLRenderTarget::Format::CUBE);
		cube.Resize(m_cubeResolution, m_cubeResolution);
		if (cube.Depth().id == 0) {
			return false;
		}

		const raylib::Vector3 eye = light.position;
		const float r = (light.range > 0.01f) ? light.range : 0.01f;

		raylib::Matrix boxView = raylib::Matrix(MatrixTranslate(-eye.x, -eye.y, -eye.z));
		raylib::Matrix boxProj = raylib::Matrix(MatrixOrtho(-r, r, -r, r, -r, r));
		m_lightFrustum.buildFromMatrix(boxView.Multiply(boxProj));
		m_queue.Clear();
		m_visibility->gatherVisible(*ctx.registry, &m_lightFrustum, m_visible,
			ctx.renderStats.culledEntities);
		RenderSystem(*ctx.registry, *ctx.assets, m_queue, m_visible, ctx.renderStats);
		m_queue.Sort();
		m_queue.BuildBatches();

		// The 6 cube faces + their up vectors (standard GL cubemap convention).
		const Vector3 dirs[6] = {
			{  1,  0,  0 }, { -1,  0,  0 }, {  0,  1,  0 },
			{  0, -1,  0 }, {  0,  0,  1 }, {  0,  0, -1 },
		};
		const Vector3 ups[6] = {
			{  0, -1,  0 }, {  0, -1,  0 }, {  0,  0,  1 },
			{  0,  0, -1 }, {  0, -1,  0 }, {  0, -1,  0 },
		};

		ScopedDepthTest depth(true);
		ScopedDepthMask mask(true);
		ScopedBackfaceCull cull(true);
		for (int face = 0; face < 6; ++face) {
			raylib::Matrix view = raylib::Matrix(MatrixLookAt(
				eye, eye.Add(dirs[face]), ups[face]));
			raylib::Matrix proj = raylib::Matrix(
				MatrixPerspective(90.0 * DEG2RAD, 1.0, m_near, (double)r));
			raylib::Matrix faceViewProj = view.Multiply(proj);

			cube.BindFace(face);
			rlClearScreenBuffers(); // clears this face's depth to 1.0 (= max distance)
			ctx.glRenderer->DrawDepth(m_queue.batches(), m_queue.batchCount(),
				*ctx.assets, faceViewProj, pointDepthShaderId, r,
				/*linearDistance*/ true, &light.position);
		}
		cube.EndCubeFace();
		return true;
	}
	bool ShadowPass::renderDirectionLightDepth(RenderContext& ctx,
		SceneLights& lights,
		const uint32_t& slot,
		LightParameter& light,
		const float& range,
		const uint32_t& depthShaderId)
	{
		if (lights.shadowCount >= (uint32_t)SceneLights::kMaxShadows) {
			return false;
		}
		raylib::Matrix lightViewProj;
		if (!buildLightMatrix(light, lightViewProj, light.range)) {
			return false; // unsupported type
		}
		GLRenderTarget& target = m_targets[slot];
		target.SetFormat(GLRenderTarget::Format::DEPTH);
		target.Resize(m_resolution, m_resolution);
		if (target.Depth().id == 0) {
			return false; // depth framebuffer failed to allocate
		}

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
			raylib::Color::Black().ClearBackground();
			ctx.glRenderer->DrawDepth(m_queue.batches(), m_queue.batchCount(),
				*ctx.assets, lightViewProj, depthShaderId, 0);
			target.Unbind();
		}
		lights.shadows[slot].lightViewProj = lightViewProj;
		lights.shadows[slot].depthTexId = target.Depth().id;
		light.shadowIndex = (int)slot;
		lights.shadowCount++;
		return true;
	}
}