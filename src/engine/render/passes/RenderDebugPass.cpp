#include "RenderDebugPass.hpp"

namespace Long {
	void RenderDebugPass::execute(RenderContext& ctx)
	{
		if (!ctx.commandDebugQueue) {
			return;
		}
		ctx.camera->BeginMode();
		ctx.commandDebugQueue->Execute(ctx.renderStats);
		ctx.camera->EndMode();
	}
}