#include "Material.hpp"
#include "rlgl.h"
#include <variant>

namespace Long {
	BaseMaterial::BaseMaterial()
	{
		// Default: receive shadows at full strength. Pushed as uniforms so shaders
		// can gate/scale the shadow term per material (ground receives, emissive
		// glow may not). Must be set here: an unset GLSL uniform reads 0.0, which
		// would make shadows vanish on materials that never touch it.
		SetUniform("u_receiveShadow", 1);
		SetUniform("u_shadowOpacity", 0.5f);
	}

	BaseMaterial::~BaseMaterial()
	{
		m_rlMaterial.shader.id = rlGetShaderIdDefault();
		m_rlMaterial.shader.locs = nullptr;
	}

	void BaseMaterial::SetUniform(const std::string& name, const UniformValue& value)
	{
		maps[name] = value;
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

	void BaseMaterial::ApplyUniforms(raylib::Shader& shader)
	{
		for (auto& [name, value] : maps) {
			int loc;
			auto it = map_locations.find(name);
			if (it != map_locations.end()) {
				loc = it->second;
			}
			else {
				loc = shader.GetLocation(name);
				map_locations[name] = loc;
			}
			if (loc < 0) {
				continue;
			}
			std::visit([&](auto&& v) {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, float>)
					shader.SetValue(loc, &v, SHADER_UNIFORM_FLOAT);
				else if constexpr (std::is_same_v<T, int>)
					shader.SetValue(loc, &v, SHADER_UNIFORM_INT);
				else if constexpr (std::is_same_v<T, raylib::Vector2>)
					shader.SetValue(loc, &v, SHADER_UNIFORM_VEC2);
				else if constexpr (std::is_same_v<T, raylib::Vector3>)
					shader.SetValue(loc, &v, SHADER_UNIFORM_VEC3);
				else if constexpr (std::is_same_v<T, raylib::Vector4>)
					shader.SetValue(loc, &v, SHADER_UNIFORM_VEC4);
				}, value);
		}
	}
}