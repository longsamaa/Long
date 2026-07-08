#include "engine/materials/DefaultMaterial.hpp"

namespace Long {

	DefaultMaterial::DefaultMaterial(uint32_t shaderId,
		raylib::Color albedo,
		raylib::Vector3 emissive,
		float emissiveIntensity,
		float metallic,
		float roughness,
		float ao) {
		SetShaderId(shaderId);
		SetColor(albedo);              // inherited from BaseMaterial
		SetEmissive(emissive, emissiveIntensity);
		SetMetallic(metallic);
		SetRoughness(roughness);
		SetAO(ao);
	}


} // namespace Long
