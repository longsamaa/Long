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

		// --- Shaders ---
		// Scan `directory` for matching <name>.vert + <name>.frag pairs and load
		// each as a shader cached under <name>. Call once after the GL context
		// exists (window created), before rendering.
		void LoadAllShaders(const std::filesystem::path& directory);

		// Look up a loaded shader id by name (the file stem). Returns Invalid if
		// not found.
		uint32_t GetShaderId(const std::string& name) const;
		raylib::Shader& GetShader(uint32_t id) { return m_shaders[id]; }
		bool IsValidShader(uint32_t id) const { return id < m_shaders.size(); }

		uint32_t AddMesh(raylib::Mesh&& mesh);
		raylib::Mesh& GetMesh(uint32_t id) { return m_meshes[id]; }
		bool IsValidMesh(uint32_t id) const { return id < m_meshes.size(); }

		// Take ownership of a polymorphic material. Returns its id.
		uint32_t AddMaterial(std::unique_ptr<BaseMaterial> material);
		BaseMaterial& GetMaterial(uint32_t id) { return *m_materials[id]; }
		bool IsValidMaterial(uint32_t id) const { return id < m_materials.size(); }

		// Create a DefaultMaterial (flat color) using the given shader. Returns id.
		uint32_t CreateDefaultMaterial(uint32_t shaderId,
									   raylib::Color color = raylib::Color::White());

	private:
		// Declared first so they are DESTROYED LAST: materials reference shaders,
		// so shaders must outlive materials.
		std::vector<raylib::Shader> m_shaders;
		std::unordered_map<std::string, uint32_t> m_shaderNameToId;
		std::vector<std::unique_ptr<BaseMaterial>> m_materials;
		std::vector<raylib::Mesh> m_meshes;
	};
} // namespace Long

#endif // !_ASSET_MANAGER_HPP_
