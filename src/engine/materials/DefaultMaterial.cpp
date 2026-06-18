#include "engine/materials/DefaultMaterial.hpp"

namespace Long {

	DefaultMaterial::DefaultMaterial(uint32_t shaderId,
		raylib::Color color) {
		SetShaderId(shaderId);
		SetColor(color);
	}

	void DefaultMaterial::SetColor(raylib::Color color) {
		raylib::Vector4 c{
			color.r / 255.0f,
			color.g / 255.0f,
			color.b / 255.0f,
			color.a / 255.0f
		};
		SetUniform("u_baseColor", c);
	}

	raylib::Material& DefaultMaterial::Apply(raylib::Shader& shader) {
		m_rlMaterial.shader = shader;
		ApplyUniforms(shader);
		return m_rlMaterial;
	}

} // namespace Long
