#pragma once
#ifndef _DEFAULT_MATERIAL_HPP_
#define _DEFAULT_MATERIAL_HPP_

#include "engine/Material.hpp"

namespace Long {
	class DefaultMaterial : public BaseMaterial {
	public:
		explicit DefaultMaterial(uint32_t shaderId,
								raylib::Color albedo = raylib::Color::White(),
								raylib::Vector3 emissive = raylib::Color::White(),
								float emissiveIntensity = 1.0f,
								float metallic = 0.0f,
								float roughness = 1.0f,
								float ao = 1.0f);
		raylib::Material& Apply(raylib::Shader& shader) override;
	};

} // namespace Long

#endif // !_DEFAULT_MATERIAL_HPP_
