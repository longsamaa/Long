#pragma once
#ifndef _EMISSIVE_MATERIAL_HPP_
#define _EMISSIVE_MATERIAL_HPP_

#include "engine/Material.hpp"

namespace Long {

	// A self-illuminating material: the shader outputs color * intensity, which can
	// exceed 1.0 so the pixels survive bloom's bright-pass and glow. Render the
	// scene into an HDR target for this to do anything (LDR clamps at 1.0).
	class EmissiveMaterial : public BaseMaterial {
	public:
		explicit EmissiveMaterial(uint32_t shaderId,
			raylib::Color color = raylib::Color::White(),
			float intensity = 4.0f);
		void SetColor(raylib::Color color);
		void SetIntensity(float intensity); // >1 makes the surface bloom
	};

} // namespace Long

#endif // !_EMISSIVE_MATERIAL_HPP_
