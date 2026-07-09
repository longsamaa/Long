#pragma once
#ifndef _GLTF_IMPORTER_HPP_
#define _GLTF_IMPORTER_HPP_
#include <filesystem>
#include <vector>
#include <map>
#include <entt/entt.hpp>
#include <raylib-cpp.hpp>
namespace Long {
	class AssetManager;
	struct GLTFTransForm {
		raylib::Vector3 pos{};
		raylib::Vector3 scale{};
		raylib::Quaternion quaternion{};
	};
	struct GLTFNode {
		std::string name; 
		int id_parent{ -1 }; 
		GLTFTransForm transform; 
		int meshId{ -1 }; //Mesh id component
		int index{ -1 };
		std::vector<int> children; 
	};
	struct ModelAsset {
		std::vector<GLTFNode> nodes{}; 
		bool IsValid() const { return !nodes.empty(); }
	};
	ModelAsset ImportGLTF(const std::filesystem::path& modelPath, AssetManager& assets);
}
#endif // !_GLTF_IMPORTER_HPP_