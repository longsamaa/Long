#pragma once
#ifndef _SKYBOX_MATERIAL_HPP_
#define _SKYBOX_MATERIAL_HPP_

#include "engine/Material.hpp"

namespace Long {
	class PointLightDepthMaterial : public BaseMaterial {
	public:
		explicit PointLightDepthMaterial();
		void Set(const raylib::Vector3& u_lightPos, const float& range); 
		raylib::Material& Apply(raylib::Shader& shader) override;
	};
} // namespace Long

#endif // !_DEFAULT_MATERIAL_HPP_