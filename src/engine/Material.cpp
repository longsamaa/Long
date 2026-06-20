#include "Material.hpp"
#include "rlgl.h"
#include <variant>

namespace Long {
	BaseMaterial::BaseMaterial()
	{
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