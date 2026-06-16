#include "engine/AssetManager.hpp"

namespace Long {

uint32_t AssetManager::AddMesh(raylib::Mesh&& mesh) {
	uint32_t id = (uint32_t)m_meshes.size();
	m_meshes.push_back(std::move(mesh));
	return id;
}

uint32_t AssetManager::AddMaterial(raylib::Material&& material) {
	uint32_t id = (uint32_t)m_materials.size();
	m_materials.push_back(std::move(material));
	return id;
}

uint32_t AssetManager::CreateDefaultMaterial(raylib::Color color) {
	raylib::Material mat; // default material (default shader)
	// Tint the diffuse/albedo map color.
	mat.GetMaps()[MATERIAL_MAP_DIFFUSE].color = color;
	return AddMaterial(std::move(mat));
}

} // namespace Long
