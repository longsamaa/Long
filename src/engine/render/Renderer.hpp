#pragma once
#ifndef _RENDERER_HPP_
#define _RENDERER_HPP_

#include "engine/render/IRenderPass.hpp"
#include "engine/render/RenderTarget.hpp"
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

		// Targets persist across frames (passes resize them as needed).
		RenderTarget m_sceneTarget;
		RenderTarget m_maskTarget;
		RenderTarget m_finalTarget;
	};

} // namespace Long

#endif // !_RENDERER_HPP_
