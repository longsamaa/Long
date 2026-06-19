#include "engine/render/passes/ScenePass.hpp"
#include "system/RenderSystem.hpp"
#include "raylib-cpp.hpp"

namespace Long {

	void ScenePass::execute(RenderContext& ctx) {
		if (!ctx.sceneTarget || !ctx.registry || !ctx.assets || !ctx.camera) {
			return;
		}

		// Match the target to the current viewport size (lazy: no-op if same).
		ctx.sceneTarget->Resize(ctx.width, ctx.height);
		if (!ctx.sceneTarget->IsValid()) {
			return;
		}

		// Render the scene into the texture instead of the screen.
		ctx.sceneTarget->Bind();
		{
			ClearBackground(DARKGRAY);
			ctx.camera->BeginMode();
			DrawGrid(20, 1.0f); // ground reference grid
			RenderSystem(*ctx.registry, *ctx.assets, ctx.renderStats);
			ctx.camera->EndMode();
		}
		ctx.sceneTarget->Unbind();
	}

}
