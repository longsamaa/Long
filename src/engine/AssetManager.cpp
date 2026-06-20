#include <format>
#include "engine/AssetManager.hpp"
#include "engine/materials/DefaultMaterial.hpp"
#include "raylib.h"
#include "Logger.hpp"

namespace fs = std::filesystem;

namespace Long {
	void AssetManager::LoadAllShaders(const fs::path& directory) {
		if (!fs::exists(directory) || !fs::is_directory(directory)) {
			TraceLog(LOG_WARNING, "Shader dir not found: %s", directory.string().c_str());
			return;
		}
		for (const auto& entry : fs::directory_iterator(directory)) {
			if (!entry.is_regular_file() || entry.path().extension() != ".vert") {
				continue;
			}
			const std::string name = entry.path().stem().string();
			fs::path frag = directory / (name + ".frag");
			if (!fs::exists(frag)) {
				Logger::TraceLog(::LOG_ERROR, std::format("Shader {} has .vert but no .frag, skipping", name.c_str()));
				continue;
			}
			raylib::Shader shader(entry.path().string(), frag.string());
			uint32_t id = (uint32_t)m_shaders.size();
			m_shaders.push_back(std::move(shader));
			m_shaderNameToId[name] = id;
			Logger::TraceLog(LOG_INFO, std::format("Loaded shader {} (id={})", name.c_str(), id));
		}
	}

	uint32_t AssetManager::GetShaderId(const std::string& name) const {
		auto it = m_shaderNameToId.find(name);
		if (it != m_shaderNameToId.end()) {
			return it->second;
		}
		else {
			Logger::TraceLog(::LOG_ERROR, std::format("Shader not found: {}", name.c_str()));
			return Invalid;
		}
	}

	uint32_t AssetManager::AddMesh(raylib::Mesh&& mesh) {
		uint32_t id = (uint32_t)m_meshes.size();
		m_meshes.push_back(std::move(mesh));
		return id;
	}

	uint32_t AssetManager::AddMaterial(std::unique_ptr<BaseMaterial> material) {
		uint32_t id = (uint32_t)m_materials.size();
		m_materials.push_back(std::move(material));
		return id;
	}

	uint32_t AssetManager::CreateDefaultMaterial(uint32_t shaderId, raylib::Color color) {
		//get shader
		raylib::Shader& shader = GetShader(shaderId);
		return AddMaterial(std::make_unique<DefaultMaterial>(shaderId, color));
	}
} // namespace Long