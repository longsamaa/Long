#include "engine/materials/EmissiveMaterial.hpp"

namespace Long {

	static raylib::Vector4 ToVec4(raylib::Color c) {
		return raylib::Vector4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
	}

	EmissiveMaterial::EmissiveMaterial(uint32_t shaderId,
		raylib::Color color, float intensity) {
		SetShaderId(shaderId);
		SetColor(color);
		SetIntensity(intensity);
	}

	void EmissiveMaterial::SetColor(raylib::Color color) {
		SetUniform("u_emissiveColor", ToVec4(color));
	}

	void EmissiveMaterial::SetIntensity(float intensity) {
		SetUniform("u_emissiveIntensity", intensity);
	}

	raylib::Material& EmissiveMaterial::Apply(raylib::Shader& shader) {
		m_rlMaterial.shader = shader;
		ApplyUniforms(shader);
		return m_rlMaterial;
	}

} // namespace Long
