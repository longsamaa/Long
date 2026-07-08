#pragma once
#ifndef _RENDERER_HPP_
#define _RENDERER_HPP_

#include "engine/render/IRenderPass.hpp"
#include "engine/render/GLRenderTarget.hpp"
#include "engine/render/RenderContext.hpp"
#include <memory>
#include <vector>

namespace Long {

	// Drives the render pipeline: owns the shared render targets and a stack of
	// passes, and runs them in order each frame. Add/remove passes to change the
	// pipeline (scene -> outline -> composite -> ...).
	class Renderer {
	public:
		Renderer() = default;

		// Append a pass to the end of the stack.
		void AddPass(std::unique_ptr<IRenderPass> pass);

		// Run every enabled pass. Wires the renderer's targets into ctx first.
		void Render(RenderContext& ctx);

	private:
		std::vector<std::unique_ptr<IRenderPass>> m_passes;
		GLRenderTarget m_sceneTarget;
		GLRenderTarget m_maskTarget;
		GLRenderTarget m_finalTarget;
		GLRenderTarget m_brightTarget;
		GLRenderTarget m_blurTarget;
		GLRenderTarget m_depthShadowTarget;
		GLRenderTarget m_ldrTarget;
		GLRenderer m_glRenderer;   // GL backend, owns the instance VBO across frames
	};

} // namespace Long

#endif // !_RENDERER_HPP_
