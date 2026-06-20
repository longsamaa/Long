#include "engine/render/Renderer.hpp"

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
		for (auto& pass : m_passes) {
			if (pass->isEnabled()) {
				pass->execute(ctx);
			}
		}
	}

} // namespace Long
