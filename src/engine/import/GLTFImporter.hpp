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
		// A glTF mesh can have MANY primitives (usually one per material); each
		// becomes its own MeshCPU/asset id. Keeping them all here (instead of a
		// single meshId) so no primitive is dropped -- createRobot makes one child
		// entity per extra primitive.
		std::vector<int> meshIds;
		int skinId{ -1 };  //Index into ModelAsset::skins, -1 = not skinned
		int index{ -1 };
		std::vector<int> children;
	};
	struct GLTFSkin {
		std::vector<int> joints;                 // node indices (in ModelAsset::nodes)
		std::vector<raylib::Matrix> inverseBind; // one per joint, same order
	};
	struct ModelAsset {
		std::vector<GLTFNode> nodes{};
		std::vector<GLTFSkin> skins{};
		bool IsValid() const { return !nodes.empty(); }
	};
	ModelAsset ImportGLTF(const std::filesystem::path& modelPath, AssetManager& assets);
}
#endif // !_GLTF_IMPORTER_HPP_