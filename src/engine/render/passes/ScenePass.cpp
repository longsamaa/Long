#include "engine/render/passes/ScenePass.hpp"
#include "system/RenderSystem.hpp"
#include "raylib-cpp.hpp"
#include "../../../engine/AssetManager.hpp"
namespace Long {

	void ScenePass::execute(RenderContext& ctx) {
		if (!ctx.sceneTarget || !ctx.registry || !ctx.assets || !ctx.camera) {
			return;
		}
		ctx.sceneTarget->Resize(ctx.width, ctx.height);
		if (!ctx.sceneTarget->IsValid()) {
			return;
		}
		// Build this frame's draw commands from the ECS, then sort them to
		// minimize GPU state changes before drawing.
		ctx.commandQueue->Clear();
		RenderSystem(*ctx.registry, *ctx.assets, *ctx.commandQueue);
		ctx.commandQueue->Sort();

		ctx.sceneTarget->Bind();
		{
			ClearBackground(DARKGRAY);
			ctx.camera->BeginMode();
			DrawGrid(20, 1.0f); // ground reference grid

			// Execute applies a material's shader/uniforms only when it changes
			// (cheap thanks to Sort) and fills the draw stats.
			ctx.commandQueue->Execute(*ctx.assets, ctx.renderStats);

			ctx.camera->EndMode();
		}
		ctx.sceneTarget->Unbind();
	}
}
