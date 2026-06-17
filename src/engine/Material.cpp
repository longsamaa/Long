#include "Material.hpp"
#include "rlgl.h"
#include <variant>

namespace Long {
	BaseMaterial::BaseMaterial()
	{
	}

	BaseMaterial::~BaseMaterial()
	{
		// m_rlMaterial does NOT own the shader -- the shader lives in the
		// AssetManager and is unloaded there. We only borrowed it for drawing.
		// Reset to the default shader id so UnloadMaterial (run by
		// raylib::Material's destructor) doesn't free the shared shader twice.
		m_rlMaterial.shader.id = rlGetShaderIdDefault();
		m_rlMaterial.shader.locs = nullptr;
	}

	void BaseMaterial::SetUniform(const std::string& name, const UniformValue& value)
	{
		// operator[] inserts a new entry or overwrites the existing one, so
		// setting the same uniform twice just updates its value.
		maps[name] = value;
	}

	void BaseMaterial::ApplyUniforms(raylib::Shader& shader)
	{
		// Push every stored uniform onto the shader. std::visit picks the right
		// SHADER_UNIFORM_* based on which type the variant currently holds.
		for (auto& [name, value] : maps) {
			int loc = shader.GetLocation(name);
			if (loc < 0) {
				continue; // uniform not present in this shader; skip silently
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