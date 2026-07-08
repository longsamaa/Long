#pragma once
#ifndef _MASK_MATERIAL_HPP_
#define _MASK_MATERIAL_HPP_

#include "engine/Material.hpp"

namespace Long {

	// Flat-color material for the selection mask: outputs a single solid color
	// (no texture, no lighting). Used by MaskPass to fill selected geometry.
	class MaskMaterial : public BaseMaterial {
	public:
		explicit MaskMaterial(uint32_t shaderId,
							  raylib::Color color = raylib::Color::White());

		void SetColor(raylib::Color color);

	};

}
#endif // !_MASK_MATERIAL_HPP_
