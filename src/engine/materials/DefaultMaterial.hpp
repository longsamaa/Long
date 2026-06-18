#pragma once
#ifndef _DEFAULT_MATERIAL_HPP_
#define _DEFAULT_MATERIAL_HPP_

#include "engine/Material.hpp"

namespace Long {
	class DefaultMaterial : public BaseMaterial {
	public:
		explicit DefaultMaterial(uint32_t shaderId,
								raylib::Color color = raylib::Color::White());
		void SetColor(raylib::Color color);
		raylib::Material& Apply(raylib::Shader& shader) override;
	};

} // namespace Long

#endif // !_DEFAULT_MATERIAL_HPP_
