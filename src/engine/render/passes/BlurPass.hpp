#pragma once
#ifndef _BLUR_PASS_HPP_
#define _BLUR_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
#include "engine/render/GLRenderTarget.hpp"
namespace Long {
	//Blur pass
	class BlurPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	public: 
		int u_radius{ 20 };
		int down_scale{ 1 };
	private:
		uint32_t m_shaderId{ UINT32_MAX };
		GLRenderTarget m_blurTarget; 
	};
}
#endif // !_BRIGHT_PASS_HPP_