#pragma once
#ifndef _MASK_PASS_HPP_
#define _MASK_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
#include "engine/materials/MaskMaterial.hpp"
#include <memory>
namespace Long {

	// Renders the selected entities as flat white on black into ctx.maskTarget.
	// OutlinePass then edge-detects this mask. Half-resolution is enough for a
	// thin outline and cheaper on weak GPUs.
	class MaskPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	private:
		std::unique_ptr<MaskMaterial> m_material; // lazily created (needs shader id)
	};

}
#endif // !_MASK_PASS_HPP_
