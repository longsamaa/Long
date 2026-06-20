#pragma once
#ifndef _SKYBOX_MATERIAL_HPP_
#define _SKYBOX_MATERIAL_HPP_

#include "engine/Material.hpp"

namespace Long {
	class SkyboxMaterial : public BaseMaterial {
	public:
		explicit SkyboxMaterial(uint32_t shaderId,
			raylib::Vector3 topColor = { 0.18f, 0.38f, 0.60f },
			raylib::Vector3 botColor = { 0.02f, 0.05f, 0.10f },
			float gradientSharpness = 1.0f);
		void SetColor(raylib::Vector3 topColor,
			raylib::Vector3 botColor,
			float gradientSharpness);
		raylib::Material& Apply(raylib::Shader& shader) override;
	};
} // namespace Long

#endif // !_DEFAULT_MATERIAL_HPP_