#pragma once
#ifndef _DEFAULT_MATERIAL_HPP_
#define _DEFAULT_MATERIAL_HPP_

#include "engine/Material.hpp"

namespace Long {

	// Simplest material: a single flat base color fed to the shader's
	// `colDiffuse` uniform. Serves as the default/fallback material and as an
	// example of deriving from BaseMaterial.
	class DefaultMaterial : public BaseMaterial {
	public:
		explicit DefaultMaterial(uint32_t shaderId,
								 raylib::Color color = raylib::Color::White());

		// Update the base color (stored as the `colDiffuse` vec4 uniform).
		void SetColor(raylib::Color color);

		raylib::Material& Apply(raylib::Shader& shader) override;
	};

} // namespace Long

#endif // !_DEFAULT_MATERIAL_HPP_
