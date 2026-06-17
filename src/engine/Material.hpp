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
		// Bind state + push uniforms for a draw. Returns the underlying
		// raylib::Material (carrying the shader) so RenderSystem can hand it to
		// mesh.Draw(). Derived classes implement the uniform setup.
		virtual raylib::Material& Apply(raylib::Shader& shader) = 0;

		void SetUniform(const std::string& name, const UniformValue& value);

		void SetShaderId(uint32_t id) { shaderId = id; }
		uint32_t GetShaderId() const { return shaderId; }

	protected:
		// Shared helper: push every stored uniform onto the shader. Derived
		// classes call this from their Apply().
		void ApplyUniforms(raylib::Shader& shader);

		uint32_t shaderId = UINT32_MAX;
		std::unordered_map<std::string, UniformValue> maps;

		// The raylib material handed to DrawMesh. We point its .shader at our
		// custom shader so raylib uses it.
		raylib::Material m_rlMaterial;
	};
}
#endif // !
