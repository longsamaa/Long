#include <format>
#include <fstream>
#include <sstream>
#include "engine/AssetManager.hpp"
#include "engine/materials/DefaultMaterial.hpp"
#include "engine/materials/WireframeMaterial.hpp"
#include "engine/materials/EmissiveMaterial.hpp"
#include "raylib.h"
#include "Logger.hpp"

namespace fs = std::filesystem;

namespace {
	// Read a whole text file into a string.
	std::string ReadFile(const fs::path& path) {
		std::ifstream f(path, std::ios::binary);
		std::ostringstream ss;
		ss << f.rdbuf();
		return ss.str();
	}

	// Insert `#define <macro>` right after the "#version ..." line so it applies
	// to the whole shader (a #define before #version is a GLSL error).
	std::string InjectDefine(const std::string& src, const std::string& macro) {
		size_t versionPos = src.find("#version");
		if (versionPos == std::string::npos) {
			return "#define " + macro + "\n" + src; // no #version (unlikely)
		}
		size_t eol = src.find('\n', versionPos);
		if (eol == std::string::npos) eol = src.size();
		return src.substr(0, eol + 1) + "#define " + macro + "\n" + src.substr(eol + 1);
	}
}

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
			raylib::Shader shader(raylib::Shader::Load(entry.path().string(), frag.string()));
			uint32_t id = (uint32_t)m_shaders.size();
			m_shaders.push_back(std::move(shader));
			m_shaderNameToId[name] = id;
			Logger::TraceLog(LOG_INFO, std::format("Loaded shader {} (id={})", name.c_str(), id));
		}
	}

	uint32_t AssetManager::LoadInstancedVariant(const fs::path& directory,
		const std::string& name) {
		fs::path vert = directory / (name + ".vert");
		fs::path frag = directory / (name + ".frag");
		if (!fs::exists(vert) || !fs::exists(frag)) {
			Logger::TraceLog(::LOG_ERROR,
				std::format("Instanced variant: {} .vert/.frag missing", name));
			return Invalid;
		}
		// Compile the SAME source but with INSTANCED defined in the vertex stage.
		std::string vs = InjectDefine(ReadFile(vert), "INSTANCED");
		std::string fs = ReadFile(frag);
		raylib::Shader shader(raylib::Shader::LoadFromMemory(vs.c_str(), fs.c_str()));
		const std::string key = name + "_instanced";
		uint32_t id = (uint32_t)m_shaders.size();
		m_shaders.push_back(std::move(shader));
		m_shaderNameToId[key] = id;
		// Link the base shader id to this instanced variant so the renderer can
		// look it up when batching.
		uint32_t baseId = GetShaderId(name);
		if (baseId != Invalid) {
			m_instancedOf[baseId] = id;
		}
		Logger::TraceLog(LOG_INFO,
			std::format("Loaded instanced shader {} (id={})", key, id));
		return id;
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

	uint32_t AssetManager::AddMesh(MeshCPU&& mesh) {
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

	uint32_t AssetManager::CreateWireframeMaterial(uint32_t shaderId,
		raylib::Color lineColor, raylib::Color faceColor, float thickness) {
		return AddMaterial(std::make_unique<WireframeMaterial>(
			shaderId, lineColor, faceColor, thickness));
	}

	uint32_t AssetManager::CreateEmissiveMaterial(uint32_t shaderId,
		raylib::Color color, float intensity) {
		return AddMaterial(std::make_unique<EmissiveMaterial>(
			shaderId, color, intensity));
	}
	//Nap thang vao registry luon 
	ModelAsset AssetManager::ImportModel(const std::filesystem::path& path)
	{
		return ImportGLTF(path, *this);
	}
} // namespace Long