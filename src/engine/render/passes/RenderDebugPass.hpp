#pragma once
#ifndef _RENDER_DEBUG_PASS_HPP_
#define _RENDER_DEBUG_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
#include "engine/render/RenderTarget.hpp"
namespace Long {
	class RenderDebugPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	};
}
#endif // !_OUTLINE_PASS_HPP_