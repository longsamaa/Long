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
	class BaseMaterial {
	public: 
		BaseMaterial();
		~BaseMaterial();
	public:
		virtual raylib::Material& Apply(raylib::Shader& shader) = 0;
		void SetUniform(const std::string& name, const UniformValue& value);
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
		void SetShaderId(uint32_t id) { shaderId = id; map_locations.clear(); }
		void ApplyShader(uint32_t id) {};
		uint32_t GetShaderId() const { return shaderId; }
	protected:
		void ApplyUniforms(raylib::Shader& shader);
		uint32_t shaderId = UINT32_MAX;
		bool castShadow{ true };
		bool receiveShadow{ true };
		std::unordered_map<std::string, UniformValue> maps;
		std::unordered_map<std::string, uint32_t> map_locations;
		raylib::Material m_rlMaterial;
	};
}
#endif // !
