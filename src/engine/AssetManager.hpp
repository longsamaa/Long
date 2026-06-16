#pragma once
#ifndef _ASSET_MANAGER_HPP_
#define _ASSET_MANAGER_HPP_

#include <raylib-cpp.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace Long {
	// Owns GPU resources and hands them out by id. Meshes and materials are
	// stored separately (not bundled in a Model) so entities can mix any mesh
	// with any material -- this is what lets you custom-shade things. Components
	// store ids (MeshFilter.meshId / MeshRenderer.materialId), never the objects.
	class AssetManager {
	public:
		static constexpr uint32_t Invalid = UINT32_MAX;

		AssetManager() = default;
		~AssetManager() = default;

		// No copies -- it owns GPU handles.
		AssetManager(const AssetManager&) = delete;
		AssetManager& operator=(const AssetManager&) = delete;

		// --- Meshes ---
		// Take ownership of a mesh (e.g. from raylib::Mesh::Cube()). Returns id.
		uint32_t AddMesh(raylib::Mesh&& mesh);
		raylib::Mesh& GetMesh(uint32_t id) { return m_meshes[id]; }
		bool IsValidMesh(uint32_t id) const { return id < m_meshes.size(); }

		// --- Materials ---
		// Take ownership of a material. Returns id.
		uint32_t AddMaterial(raylib::Material&& material);
		raylib::Material& GetMaterial(uint32_t id) { return m_materials[id]; }
		bool IsValidMaterial(uint32_t id) const { return id < m_materials.size(); }

		// Create a default material tinted with a color. Returns its id.
		uint32_t CreateDefaultMaterial(raylib::Color color = raylib::Color::White());

	private:
		std::vector<raylib::Mesh> m_meshes;
		std::vector<raylib::Material> m_materials;
	};
} // namespace Long

#endif // !_ASSET_MANAGER_HPP_
