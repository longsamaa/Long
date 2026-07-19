#include "engine/render/Renderer.hpp"
#include "helpers/TimerHelper.hpp"
#include "engine/Logger.hpp"
#include <typeinfo>
namespace Long {
	void Renderer::AddPass(std::unique_ptr<IRenderPass> pass) {
		// Profiler display name from RTTI: "class Long::ShadowPass" -> "ShadowPass".
		std::string name = typeid(*pass).name();
		if (const size_t p = name.rfind("::"); p != std::string::npos) {
			name = name.substr(p + 2);
		}
		m_passNames.push_back(std::move(name));
		m_passes.push_back(std::move(pass));
	}
	void Renderer::Render(RenderContext& ctx) {
		m_sceneTarget.SetFormat(RenderTarget::Format::HDR);
		m_finalTarget.SetFormat(RenderTarget::Format::HDR);
		m_brightTarget.SetFormat(RenderTarget::Format::HDR);
		m_blurTarget.SetFormat(RenderTarget::Format::HDR);
		m_depthShadowTarget.SetFormat(RenderTarget::Format::DEPTH);
		ctx.sceneTarget = &m_sceneTarget;
		ctx.maskTarget = &m_maskTarget;
		ctx.finalTarget = &m_finalTarget;
		ctx.brightTarget = &m_brightTarget;
		ctx.blurTarget = &m_blurTarget;
		ctx.ldrTarget = &m_ldrTarget;
		ctx.shadow_depth_target = &m_depthShadowTarget;
		ctx.glRenderer = &m_glRenderer;
		ctx.renderStats.Reset();
		const auto tRender = Time::now();
		for (size_t i = 0; i < m_passes.size(); ++i) {
			if (!m_passes[i]->isEnabled()) {
				continue;
			}
			const auto tPass = Time::now();
			m_passes[i]->execute(ctx);
			ctx.renderStats.renderPassCalls++;
			if (ctx.renderStats.passCount < RenderStats::kMaxPasses) {
				const int slot = ctx.renderStats.passCount++;
				ctx.renderStats.msPass[slot] = Time::elapsedMs(tPass);
				ctx.renderStats.passName[slot] = m_passNames[i].c_str();
			}
		}
		ctx.renderStats.msRenderTotal = Time::elapsedMs(tRender);
	}
} // namespace Long