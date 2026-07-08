#include "Material.hpp"

namespace Long {
	BaseMaterial::BaseMaterial()
	{
		SetUniform("u_receiveShadow", 1);
		SetUniform("u_shadowOpacity", 0.8f);
		// PBR surface defaults for EVERY material: an unset GLSL uniform reads
		// 0.0, which would mean mirror-smooth (roughness 0), zero ambient (ao 0).
		// Matte dielectric is the safe baseline; subclasses override as needed.
		SetMetallic(0.0f);
		SetRoughness(0.8f);
		SetAO(1.0f);
		SetEmissive({ 0.0f, 0.0f, 0.0f }, 0.0f);
	}

	void BaseMaterial::SetUniform(const std::string& name, const UniformValue& value)
	{
		m_uniforms[name] = value;
	}

	void BaseMaterial::SetColor(raylib::Color color)
	{
		raylib::Vector4 c{
			color.r / 255.0f,
			color.g / 255.0f,
			color.b / 255.0f,
			color.a / 255.0f
		};
		SetUniform("u_baseColor", c);
	}

	void BaseMaterial::SetEmissive(raylib::Vector3 color, float intensity)
	{
		SetUniform("u_emissive", color);
		SetUniform("u_emissiveIntensity", intensity);
	}

	void BaseMaterial::SetMetallic(float metallic)
	{
		SetUniform("u_metallic", metallic);
	}

	void BaseMaterial::SetRoughness(float roughness)
	{
		SetUniform("u_roughness", roughness);
	}

	void BaseMaterial::SetAO(float ao)
	{
		SetUniform("u_ao", ao);
	}
}
