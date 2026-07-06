//#pragma once 
//#ifndef _GLTF_IMPORTER_HPP_
//#define _GLTF_IMPORTER_HPP_
//#include <filesystem>
//#include <vector>
//namespace Long {
//	class AssetManager; 
//	// Meshes registered into the AssetManager, one entry PER glTF PRIMITIVE
//	// (a glTF mesh can hold several primitives, each with its own material).
//	// The three vectors are parallel:
//	//   meshIds[i]       - id in the AssetManager
//	//   gltfMeshIndex[i] - which model.meshes[] it came from (node.mesh lookup)
//	//   meshMaterial[i]  - glTF material index (-1 = none; materials done later)
//	struct ModelAsset {
//		std::vector<uint32_t> meshIds{};
//		std::vector<int> gltfMeshIndex{};
//		std::vector<int> meshMaterial{};
//		bool IsValid() const { return !meshIds.empty(); }
//	};
//	ModelAsset ImportGLTF(const std::filesystem::path& modelPath, AssetManager& assets); 
//}
//#endif // !_GLTF_IMPORTER_HPP_
