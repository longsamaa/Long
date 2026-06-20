#include "engine/render/passes/ScenePass.hpp"
#include "system/RenderSystem.hpp"
#include "raylib-cpp.hpp"
#include "engine/AssetManager.hpp"
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
		ctx.commandDebugQueue->Clear(); 
		RenderSystem(*ctx.registry, *ctx.assets, *ctx.commandQueue);
		ctx.commandQueue->Sort();
		ctx.sceneTarget->Bind();
		{
			::ClearBackground(DARKGRAY);
			ctx.commandDebugQueue->Submit(GridCommand{ 20,1.0f });
			ctx.camera->BeginMode();
			ctx.commandQueue->Execute(*ctx.assets, ctx.renderStats);
			ctx.commandDebugQueue->Execute(ctx.renderStats); 
			ctx.camera->EndMode();
		}
		ctx.renderStats.renderPassCalls++;
		ctx.sceneTarget->Unbind();
	}
}