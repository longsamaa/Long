#include "engine/render/passes/ScenePass.hpp"
#include "raylib-cpp.hpp"
#include "engine/AssetManager.hpp"
#include "engine/Environment.hpp"
#include "helpers/TimerHelper.hpp"
namespace Long {
	void ScenePass::execute(RenderContext& ctx) {
		if (!ctx.sceneTarget || !ctx.registry || !ctx.assets || !ctx.camera) {
			return;
		}

		ctx.sceneTarget->Resize(ctx.width, ctx.height);
		if (!ctx.sceneTarget->IsValid()) {
			return;
		}
		// Hemisphere ambient follows the skybox gradient so PBR ambient matches
		// what surrounds the scene (updates live when the user edits the sky).
		if (ctx.environment && ctx.lights) {
			ctx.lights->ambientSky = ctx.environment->topColor;
			ctx.lights->ambientGround = ctx.environment->bottomColor;
		}
		// Batches were built by ScenePreparePass; here we just draw them from the
		// camera's point of view (color), sampling the shadow map ShadowPass filled.
		ctx.sceneTarget->Bind();
		{
			raylib::Color::DarkGray().ClearBackground();
			ctx.camera->BeginMode();
			auto t0 = Time::now();
			ctx.glRenderer->DrawBatches(ctx.commandQueue->batches(),
				ctx.commandQueue->batchCount(), *ctx.assets, ctx.renderStats, ctx.lights);
			ctx.renderStats.msExecute = Time::elapsedMs(t0);
			auto tSky = Time::now();
			if (ctx.environment) {
				ctx.environment->DrawSkybox(*ctx.camera, *ctx.glRenderer);
			}
			ctx.renderStats.msSkybox = Time::elapsedMs(tSky);
			ctx.camera->EndMode();
		}
		auto tUnbind = Time::now();
		ctx.sceneTarget->Unbind();
		ctx.renderStats.msUnbind = Time::elapsedMs(tUnbind);
	}
}
