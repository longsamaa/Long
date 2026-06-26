#include "engine/render/Renderer.hpp"
#include "helpers/TimerHelper.hpp"

namespace Long {

	void Renderer::AddPass(std::unique_ptr<IRenderPass> pass) {
		m_passes.push_back(std::move(pass));
	}

	void Renderer::Render(RenderContext& ctx) {
		// Hand the renderer-owned targets to the passes via the context.
		ctx.sceneTarget = &m_sceneTarget;
		ctx.maskTarget  = &m_maskTarget;
		ctx.finalTarget = &m_finalTarget;
		ctx.renderStats.Reset();

		const auto tRender = Time::now();
		int idx = 0;
		for (auto& pass : m_passes) {
			if (pass->isEnabled()) {
				auto tPass = Time::now();
				pass->execute(ctx);
				if (idx < RenderStats::kMaxPasses) {
					ctx.renderStats.msPass[idx++] = Time::elapsedMs(tPass);
				}
			}
		}
		ctx.renderStats.passCount = idx;
		ctx.renderStats.msRenderTotal = Time::elapsedMs(tRender);
	}

} // namespace Long
