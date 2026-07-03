#include "engine/render/passes/ScenePass.hpp"
#include "system/RenderSystem.hpp"
#include "raylib-cpp.hpp"
#include "engine/AssetManager.hpp"
#include "engine/Environment.hpp"
#include "helpers/TimerHelper.hpp"
#include "engine/visibility/FrustumCulling.hpp"
namespace Long {
	void ScenePass::execute(RenderContext& ctx) {
		if (!ctx.sceneTarget || !ctx.registry || !ctx.assets || !ctx.camera) {
			return;
		}

		ctx.sceneTarget->Resize(ctx.width, ctx.height);
		if (!ctx.sceneTarget->IsValid()) {
			return;
		}
		if (ctx.frustum)
		{
			ctx.frustum->update();
		}
		auto t0 = Time::now();
		m_visibility->gatherVisible(*ctx.registry, ctx.frustum, m_visible,
			ctx.renderStats.culledEntities);
		RenderSystem(*ctx.registry, *ctx.assets, *ctx.commandQueue, m_visible, ctx.renderStats);
		ctx.renderStats.msRenderSystem = Time::elapsedMs(t0);
		t0 = Time::now();
		ctx.commandQueue->Sort();
		ctx.renderStats.msSort = Time::elapsedMs(t0);
		t0 = Time::now();
		ctx.commandQueue->BuildBatches();
		ctx.renderStats.msBuildBatches = Time::elapsedMs(t0);
		ctx.sceneTarget->Bind();
		{
			raylib::Color::DarkGray().ClearBackground();
			ctx.camera->BeginMode();
			t0 = Time::now();
			ctx.commandQueue->Execute(*ctx.assets, ctx.renderStats,ctx.lights);
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