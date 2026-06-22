#pragma once
#ifndef _ASSET_MANAGER_HPP_
#define _ASSET_MANAGER_HPP_
#include <raylib-cpp.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include "engine/Material.hpp"

namespace Long {
	class AssetManager {
	public:
		static constexpr uint32_t Invalid = UINT32_MAX;

		AssetManager() = default;
		~AssetManager() = default;

		AssetManager(const AssetManager&) = delete;
		AssetManager& operator=(const AssetManager&) = delete;
		//Load All Shader
		void LoadAllShaders(const std::filesystem::path& directory);
		uint32_t GetShaderId(const std::string& name) const;
		// Load a second copy of `name`'s shader with `#define INSTANCED` injected,
		// registered as `<name>_instanced`. Returns its id (or Invalid).
		uint32_t LoadInstancedVariant(const std::filesystem::path& directory,
			const std::string& name);
		// Returns the instanced shader id for a base shader id, or Invalid if the
		// shader has no instanced variant.
		uint32_t GetInstancedShaderId(uint32_t baseShaderId) const {
			auto it = m_instancedOf.find(baseShaderId);
			return (it != m_instancedOf.end()) ? it->second : Invalid;
		}
		raylib::Shader& GetShader(uint32_t id) {
			return m_shaders[id];
		}
		bool IsValidShader(uint32_t id) const { return id < m_shaders.size(); }

		uint32_t AddMesh(raylib::Mesh&& mesh);
		raylib::Mesh& GetMesh(uint32_t id) { return m_meshes[id]; }
		bool IsValidMesh(uint32_t id) const { return id < m_meshes.size(); }
		uint32_t AddMaterial(std::unique_ptr<BaseMaterial> material);
		BaseMaterial& GetMaterial(uint32_t id) { return *m_materials[id]; }
		bool IsValidMaterial(uint32_t id) const { return id < m_materials.size(); }
		uint32_t CreateDefaultMaterial(uint32_t shaderId,
			raylib::Color color = raylib::Color::White());
		size_t materialCount() { return m_materials.size(); };
	private:
		std::vector<raylib::Shader> m_shaders;
		std::unordered_map<std::string, uint32_t> m_shaderNameToId;
		std::unordered_map<uint32_t, uint32_t> m_instancedOf; // base shader id -> instanced id
		std::vector<std::unique_ptr<BaseMaterial>> m_materials;
		std::vector<raylib::Mesh> m_meshes;
	};
} // namespace Long

#endif // !_ASSET_MANAGER_HPP_