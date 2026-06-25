#include "engine/render/passes/ScenePass.hpp"
#include "system/RenderSystem.hpp"
#include "raylib-cpp.hpp"
#include "engine/AssetManager.hpp"
#include "engine/Environment.hpp"
#include <chrono>
namespace Long {
	// Steady, high-resolution clock so per-stage timings aren't affected by wall-
	// clock adjustments. Returns milliseconds elapsed since `from`.
	using Clock = std::chrono::high_resolution_clock;
	static double MsSince(Clock::time_point from) {
		return std::chrono::duration<double, std::milli>(Clock::now() - from).count();
	}

	void ScenePass::execute(RenderContext& ctx) {
		if (!ctx.sceneTarget || !ctx.registry || !ctx.assets || !ctx.camera) {
			return;
		}
		ctx.sceneTarget->Resize(ctx.width, ctx.height);
		if (!ctx.sceneTarget->IsValid()) {
			return;
		}
		// Per-stage CPU timing so the Profiler can show where a frame's time goes.
		const auto tPassStart = Clock::now();

		ctx.commandQueue->Clear();
		ctx.commandDebugQueue->Clear();

		auto t0 = Clock::now();
		RenderSystem(*ctx.registry, *ctx.assets, *ctx.commandQueue);
		ctx.renderStats.msRenderSystem = MsSince(t0);

		t0 = Clock::now();
		ctx.commandQueue->Sort();
		ctx.renderStats.msSort = MsSince(t0);

		t0 = Clock::now();
		ctx.commandQueue->BuildBatches();
		ctx.renderStats.msBuildBatches = MsSince(t0);

		ctx.sceneTarget->Bind();
		{
			::ClearBackground(raylib::Color::DarkGray());
			ctx.commandDebugQueue->Submit(GridCommand{ 20,1.0f });
			ctx.camera->BeginMode();

			t0 = Clock::now();
			ctx.commandQueue->Execute(*ctx.assets, ctx.renderStats);
			ctx.renderStats.msExecute = MsSince(t0);

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
		ctx.renderStats.msScenePass = MsSince(tPassStart);
	}
}