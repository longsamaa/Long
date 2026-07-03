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
		// Batches were built by ScenePreparePass; here we just draw them from the
		// camera's point of view (color), sampling the shadow map ShadowPass filled.
		ctx.sceneTarget->Bind();
		{
			raylib::Color::DarkGray().ClearBackground();
			ctx.camera->BeginMode();
			auto t0 = Time::now();
			ctx.commandQueue->Execute(*ctx.assets, ctx.renderStats, ctx.lights);
			ctx.renderStats.msExecute = Time::elapsedMs(t0);
			auto tSky = Time::now();
			if (ctx.environment) {
				ctx.environment->DrawSkybox(*ctx.camera);
			}
			ctx.renderStats.msSkybox = Time::elapsedMs(tSky);
			ctx.camera->EndMode();
		}
		auto tUnbind = Time::now();
		ctx.sceneTarget->Unbind();
		ctx.renderStats.msUnbind = Time::elapsedMs(tUnbind);
	}
}
