#include "engine/render/Renderer.hpp"
#include "helpers/TimerHelper.hpp"
#include "engine/Logger.hpp"
namespace Long {
	void Renderer::AddPass(std::unique_ptr<IRenderPass> pass) {
		m_passes.push_back(std::move(pass));
	}
	void Renderer::Render(RenderContext& ctx) {
		// Log each pass on the FIRST render only, so a hang shows which pass died.
		static bool firstRender = true;
		m_sceneTarget.SetFormat(RenderTarget::Format::HDR);
		m_finalTarget.SetFormat(RenderTarget::Format::HDR);
		m_brightTarget.SetFormat(RenderTarget::Format::HDR);
		m_blurTarget.SetFormat(RenderTarget::Format::HDR);
		ctx.sceneTarget = &m_sceneTarget;
		ctx.maskTarget  = &m_maskTarget;
		ctx.finalTarget = &m_finalTarget;
		ctx.brightTarget = &m_brightTarget;
		ctx.blurTarget = &m_blurTarget;
		ctx.ldrTarget = &m_ldrTarget; 
		ctx.renderStats.Reset();
		const auto tRender = Time::now();
		for (auto& pass : m_passes) {
			if (pass->isEnabled()) {
				auto tPass = Time::now();
				pass->execute(ctx);
				ctx.renderStats.renderPassCalls++;
			}
		}
		ctx.renderStats.msRenderTotal = Time::elapsedMs(tRender);
		firstRender = false;
	}

} // namespace Long
