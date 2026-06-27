#pragma once
#ifndef _BLOOM_COMPOSITE_PASS_HPP_
#define _BLOOM_COMPOSITE_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
namespace Long {
	class BloomCompositePass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	public: 
		float u_bloomStrength{ 0.5f }; 
	private:
		uint32_t m_shaderId{ UINT32_MAX };
	};

}
#endif // !_BLOOM_COMPOSITE_PASS_HPP_
