#pragma once
#ifndef _DEFAULT_MATERIAL_HPP_
#define _DEFAULT_MATERIAL_HPP_

#include "engine/Material.hpp"

namespace Long {
	class DefaultMaterial : public BaseMaterial {
	public:
		// Emissive defaults to BLACK/0: shaders that read u_emissive (pbr.frag)
		// must not glow unless asked to. Roughness 0.25: at 0.5 a dielectric's
		// 4% reflectance spreads so wide the highlight is invisible on a bright
		// albedo -- 0.25 gives the classic glossy-plastic hotspot.
		explicit DefaultMaterial(uint32_t shaderId,
								raylib::Color albedo = raylib::Color::White(),
								raylib::Vector3 emissive = { 0.0f, 0.0f, 0.0f },
								float emissiveIntensity = 0.0f,
								float metallic = 0.0f,
								float roughness = 0.25f,
								float ao = 1.0f);
	};

} // namespace Long

#endif // !_DEFAULT_MATERIAL_HPP_
