#pragma once
#ifndef _MATERIAL_HPP_
#define _MATERIAL_HPP_
#include <unordered_map>
#include <variant>
#include <string>
#include <raylib-cpp.hpp>
namespace Long {
	using UniformValue = std::variant<
		float,            // SHADER_UNIFORM_FLOAT
		int,              // SHADER_UNIFORM_INT
		raylib::Vector2,          // SHADER_UNIFORM_VEC2
		raylib::Vector3,          // SHADER_UNIFORM_VEC3
		raylib::Vector4           // SHADER_UNIFORM_VEC4
	>;

	// PURE CPU-side material description: a shader id + a named parameter table
	// + shadow flags. It knows NOTHING about how a backend consumes it -- no GL
	// uniform locations, no raylib::Material, no Apply(). The render backend
	// (GLRenderer::ApplyMaterial today, a VkRenderer's descriptor writes later)
	// resolves the names its own way and keeps its own caches.
	//
	// Subclasses (DefaultMaterial, WireframeMaterial, ...) are just convenience
	// constructors/setters that fill the parameter table.
	class BaseMaterial {
	public:
		BaseMaterial();
		virtual ~BaseMaterial() = default;
	public:
		void SetUniform(const std::string& name, const UniformValue& value);
		// Backend reads the whole table when applying the material.
		const std::unordered_map<std::string, UniformValue>& Uniforms() const {
			return m_uniforms;
		}
		// Common PBR-ish surface params every material carries. Stored as u_*
		// uniforms; shaders that don't declare them just resolve to loc -1.
		void SetColor(raylib::Color color);
		void SetEmissive(raylib::Vector3 color, float intensity);
		void SetMetallic(float metallic);
		void SetRoughness(float roughness);
		void SetAO(float ao);
		// Shadow behaviour lives on the material (it's about how the surface is
		// shaded / drawn). castShadow is queried by the depth pass; receiveShadow
		// is pushed to the shader as the u_receiveShadow uniform.
		void SetCastShadow(bool cast) { castShadow = cast; }
		bool CastsShadow() const { return castShadow; }
		void SetReceiveShadow(bool receive) {
			receiveShadow = receive;
			SetUniform("u_receiveShadow", receive ? 1 : 0);
		}
		bool ReceivesShadow() const { return receiveShadow; }
		// How dark received shadows are on this surface: 0 = invisible, 1 = full.
		void SetShadowOpacity(float opacity) {
			SetUniform("u_shadowOpacity", opacity);
		}
		void SetShaderId(uint32_t id) { shaderId = id; }
		uint32_t GetShaderId() const { return shaderId; }
	protected:
		uint32_t shaderId = UINT32_MAX;
		bool castShadow{ true };
		bool receiveShadow{ true };
		std::unordered_map<std::string, UniformValue> m_uniforms;
	};
}
#endif // !_MATERIAL_HPP_
