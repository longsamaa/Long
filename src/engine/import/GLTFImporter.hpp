#pragma once
#ifndef _GLTF_IMPORTER_HPP_
#define _GLTF_IMPORTER_HPP_
#include <filesystem>
#include <vector>
#include <map>
namespace Long {
	class AssetManager;
	struct ModelAsset {
		std::vector<uint32_t> meshIds{};
		std::map<uint32_t, std::string> meshName{};
		std::vector<int> gltfMeshIndex{};
		std::vector<int> meshMaterial{};
		bool IsValid() const { return !meshIds.empty(); }
	};
	ModelAsset ImportGLTF(const std::filesystem::path& modelPath, AssetManager& assets);
}
#endif // !_GLTF_IMPORTER_HPP_