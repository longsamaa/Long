#include "RenderDebugPass.hpp"

namespace Long {
	void RenderDebugPass::execute(RenderContext& ctx)
	{
		if (!ctx.commandDebugQueue || !ctx.glRenderer || !ctx.assets || !ctx.camera) {
			return;
		}
		// Flatten helper commands into a line list (CPU), then one GL_LINES draw
		// through the backend -- no raylib immediate-mode DrawLine3D/DrawGrid.
		ctx.commandDebugQueue->BuildLines(ctx.renderStats);
		ctx.camera->BeginMode();
		ctx.glRenderer->DrawDebugLines(*ctx.assets, ctx.commandDebugQueue->Lines());
		ctx.camera->EndMode();
	}
}