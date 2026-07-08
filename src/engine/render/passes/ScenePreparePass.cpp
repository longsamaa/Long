#include "engine/render/passes/ScenePreparePass.hpp"
#include "system/RenderSystem.hpp"
#include "engine/AssetManager.hpp"
#include "helpers/TimerHelper.hpp"
#include "engine/visibility/FrustumCulling.hpp"

namespace Long {
	void ScenePreparePass::execute(RenderContext& ctx) {
		if (!ctx.registry || !ctx.assets || !ctx.commandQueue) {
			return;
		}
		// Cull against the CAMERA frustum: batches feed both the camera color pass
		// and the shadow depth pass. (A tighter light-frustum cull would need its
		// own gather; here we share one visible set for simplicity.)
		if (ctx.frustum) {
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
	}
}