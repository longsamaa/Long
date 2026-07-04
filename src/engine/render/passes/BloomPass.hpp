#pragma once
#ifndef _BLOOM_PASS_HPP_
#define _BLOOM_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
#include "engine/render/RenderTarget.hpp"
#include <array>
namespace Long {
	class BloomPass : public IRenderPass {
	public:
		static constexpr int kMipCount = 6;
		void execute(RenderContext& ctx) override;
		float upsampleRadius{ 1.0f };
	private:
		void ensureMips(uint32_t baseW, uint32_t baseH);

		uint32_t m_downShaderId{ UINT32_MAX };
		uint32_t m_upShaderId{ UINT32_MAX };
		std::array<RenderTarget, kMipCount> m_mips; // m_mips[0] = largest
		uint32_t m_baseW{ 0 }, m_baseH{ 0 };
	};
}
#endif // !_BLOOM_PASS_HPP_