#include "engine/render/passes/ScenePass.hpp"
#include "system/RenderSystem.hpp"
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

		const auto tPassStart = Time::now();
		ctx.commandQueue->Clear();
		ctx.commandDebugQueue->Clear();

		auto t0 = Time::now();
		RenderSystem(*ctx.registry, *ctx.assets, *ctx.commandQueue, ctx.frustum, ctx.renderStats);
		ctx.renderStats.msRenderSystem = Time::elapsedMs(t0);

		t0 = Time::now();
		ctx.commandQueue->Sort();
		ctx.renderStats.msSort = Time::elapsedMs(t0);

		t0 = Time::now();
		ctx.commandQueue->BuildBatches();
		ctx.renderStats.msBuildBatches = Time::elapsedMs(t0);

		ctx.sceneTarget->Bind();
		{
			::ClearBackground(raylib::Color::DarkGray());
			ctx.commandDebugQueue->Submit(GridCommand{ 20,1.0f });
			ctx.camera->BeginMode();

			t0 = Time::now();
			ctx.commandQueue->Execute(*ctx.assets, ctx.renderStats);
			ctx.renderStats.msExecute = Time::elapsedMs(t0);

			// Skybox after opaque geometry: it writes no depth and sits at the
			// far plane, so it only fills pixels the scene didn't cover.
			ctx.commandDebugQueue->Execute(ctx.renderStats);
			if (ctx.environment) {
				ctx.environment->DrawSkybox(*ctx.camera);
			}
			// Gizmo is drawn later by GizmoPass (overlay on the screen), not here
			// in the render texture -- that keeps its mouse picking aligned.
			ctx.camera->EndMode();
		}
		ctx.renderStats.renderPassCalls++;
		ctx.sceneTarget->Unbind();
		ctx.renderStats.msScenePass = Time::elapsedMs(tPassStart);
	}
}