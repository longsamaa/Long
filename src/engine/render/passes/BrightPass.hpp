#pragma once
#ifndef _BRIGHT_PASS_HPP_
#define _BRIGHT_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
namespace Long {
	//Bright pass
	class BrightPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
		float u_threshold{ 1.0f };
		float u_softKnee{ 0.5f };
	public: 
		uint32_t down_scale{ 1 };
	private:
		uint32_t m_shaderId{ UINT32_MAX };
	};
}
#endif // !_BRIGHT_PASS_HPP_