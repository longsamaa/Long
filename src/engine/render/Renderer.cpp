#include "engine/render/Renderer.hpp"
#include <chrono>

namespace Long {

	void Renderer::AddPass(std::unique_ptr<IRenderPass> pass) {
		m_passes.push_back(std::move(pass));
	}

	void Renderer::Render(RenderContext& ctx) {
		using Clock = std::chrono::high_resolution_clock;
		auto ms = [](Clock::time_point from) {
			return std::chrono::duration<double, std::milli>(Clock::now() - from).count();
		};
		// Hand the renderer-owned targets to the passes via the context.
		ctx.sceneTarget = &m_sceneTarget;
		ctx.maskTarget  = &m_maskTarget;
		ctx.finalTarget = &m_finalTarget;
		ctx.renderStats.Reset();

		const auto tRender = Clock::now();
		int idx = 0;
		for (auto& pass : m_passes) {
			if (pass->isEnabled()) {
				auto tPass = Clock::now();
				pass->execute(ctx);
				if (idx < RenderStats::kMaxPasses) {
					ctx.renderStats.msPass[idx++] = ms(tPass);
				}
			}
		}
		ctx.renderStats.passCount = idx;
		ctx.renderStats.msRenderTotal = ms(tRender);
	}

} // namespace Long
