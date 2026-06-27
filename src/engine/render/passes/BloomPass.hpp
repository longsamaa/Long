#pragma once
#ifndef _BLOOM_PASS_HPP_
#define _BLOOM_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
#include "engine/render/RenderTarget.hpp"
#include <array>
namespace Long {

	// Mip-chain bloom (Jimenez / Call of Duty 2014). Replaces the single separable
	// blur: it progressively downsamples ctx.brightTarget into a chain of ever-
	// smaller mips, then upsamples back up adding each mip into the next, producing a
	// soft, wide, multi-radius glow. The final result is written to ctx.blurTarget so
	// BloomCompositePass can read it unchanged.
	class BloomPass : public IRenderPass {
	public:
		static constexpr int kMipCount = 6; // number of downsample levels
		void execute(RenderContext& ctx) override;
		float upsampleRadius{ 1.0f }; // tent spread when upsampling
	private:
		void ensureMips(uint32_t baseW, uint32_t baseH);

		uint32_t m_downShaderId{ UINT32_MAX };
		uint32_t m_upShaderId{ UINT32_MAX };
		std::array<RenderTarget, kMipCount> m_mips; // m_mips[0] = largest
		uint32_t m_baseW{ 0 }, m_baseH{ 0 };        // size mips were built for
	};

}
#endif // !_BLOOM_PASS_HPP_
