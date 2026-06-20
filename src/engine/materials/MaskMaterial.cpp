#include "engine/materials/MaskMaterial.hpp"

namespace Long {

	MaskMaterial::MaskMaterial(uint32_t shaderId, raylib::Color color) {
		SetShaderId(shaderId);
		SetColor(color);
	}

	void MaskMaterial::SetColor(raylib::Color color) {
		raylib::Vector4 c{
			color.r / 255.0f,
			color.g / 255.0f,
			color.b / 255.0f,
			color.a / 255.0f
		};
		SetUniform("u_maskColor", c);
	}

	raylib::Material& MaskMaterial::Apply(raylib::Shader& shader) {
		m_rlMaterial.shader = shader;
		ApplyUniforms(shader);
		return m_rlMaterial;
	}

}
