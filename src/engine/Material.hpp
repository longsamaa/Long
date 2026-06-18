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
		void SetShaderId(uint32_t id) { shaderId = id; map_locations.clear(); }
		uint32_t GetShaderId() const { return shaderId; }
	protected:
		void ApplyUniforms(raylib::Shader& shader);
		uint32_t shaderId = UINT32_MAX;
		std::unordered_map<std::string, UniformValue> maps;
		std::unordered_map<std::string, uint32_t> map_locations; 
		raylib::Material m_rlMaterial;
	};
}
#endif // !
