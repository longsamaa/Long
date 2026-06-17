#include "engine/materials/DefaultMaterial.hpp"

namespace Long {

	DefaultMaterial::DefaultMaterial(uint32_t shaderId, raylib::Color color) {
		SetShaderId(shaderId);
		SetColor(color);
	}

	void DefaultMaterial::SetColor(raylib::Color color) {
		// u_baseColor is OUR uniform (not a raylib-managed name like colDiffuse),
		// so pushing it via SetUniform/ApplyUniforms sticks -- raylib won't
		// overwrite it. Shaders want vec4 in 0..1; raylib::Color is 0..255.
		raylib::Vector4 c{
			color.r / 255.0f,
			color.g / 255.0f,
			color.b / 255.0f,
			color.a / 255.0f
		};
		SetUniform("u_baseColor", c);
	}

	raylib::Material& DefaultMaterial::Apply(raylib::Shader& shader) {
		// Make raylib use our shader for this material, then push uniforms.
		m_rlMaterial.shader = shader;
		ApplyUniforms(shader);
		return m_rlMaterial;
	}

} // namespace Long
