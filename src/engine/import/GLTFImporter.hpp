#pragma once
#ifndef _GLTF_IMPORTER_HPP_
#define _GLTF_IMPORTER_HPP_
#include <filesystem>
#include <vector>
#include <map>
#include <entt/entt.hpp>
#include <raylib-cpp.hpp>
#include "core/MeshCPU.hpp"
namespace Long {
	struct GLTFTransForm {
		raylib::Vector3 pos{};
		raylib::Vector3 scale{};
		raylib::Quaternion quaternion{};
	};
	struct GLTFNode {
		std::string name;
		int id_parent{ -1 };
		GLTFTransForm transform;
		std::vector<int> meshIds; //LOCAL indices into ModelAsset::meshes
		int skinId{ -1 };  //Index into ModelAsset::skins, -1 = not skinned
		int index{ -1 };
		std::vector<int> children;
	};
	struct GLTFSkin {
		std::vector<int> joints;                 // node indices (in ModelAsset::nodes)
		std::vector<raylib::Matrix> inverseBind; // one per joint, same order
	};
	// One animated property (translation/rotation/scale) of one node.
	// times/values are parallel arrays: values is vec3Keys for translation/scale,
	// quatKeys for rotation (the other vector stays empty).
	struct GLTFAnimChannel {
		enum class Path : uint8_t { Translation, Rotation, Scale };
		enum class Interp : uint8_t { Linear, Step }; // CUBICSPLINE imported as Linear
		int node{ -1 };                           // index into ModelAsset::nodes
		Path path{ Path::Translation };
		Interp interp{ Interp::Linear };
		std::vector<float> times;                 // keyframe times (seconds, ascending)
		std::vector<raylib::Vector3> vec3Keys;    // translation/scale keys
		std::vector<raylib::Quaternion> quatKeys; // rotation keys (unit quats)
	};
	struct GLTFAnimationClip {
		std::string name;
		float duration{ 0.0f };                   // max keyframe time (seconds)
		std::vector<GLTFAnimChannel> channels;
	};
	// Decoded image, normalized to RGBA8 by the raylib image-loader callback.
	// CPU-side only; the AssetManager uploads it at instantiation time.
	struct GLTFImage {
		int width{ 0 };
		int height{ 0 };
		std::vector<unsigned char> pixels; // width * height * 4
	};
	// PBR metallic-roughness material: factors + LOCAL indices into
	// ModelAsset::images (-1 = that map is absent).
	struct GLTFMaterial {
		raylib::Vector4 baseColorFactor{ 1.0f, 1.0f, 1.0f, 1.0f };
		float metallicFactor{ 1.0f };
		float roughnessFactor{ 1.0f };
		raylib::Vector3 emissiveFactor{ 0.0f, 0.0f, 0.0f };
		int baseColorImage{ -1 };
		int metallicRoughnessImage{ -1 }; // glTF packing: G = roughness, B = metallic
		int emissiveImage{ -1 };
		int occlusionImage{ -1 };         // R = ambient occlusion
		bool doubleSided{ false };        // true -> render both faces (CullMode::None)
	};
	// Self-contained import result: meshes live IN the asset and node.meshIds
	// are LOCAL indices into `meshes` (0..meshes.size()-1). Registration with the
	// AssetManager (local -> global id remap) happens at instantiation time in
	// ImportGLTFToScene, so this struct has no runtime state and can be
	// serialized to disk as-is (asset cooking) or instantiated repeatedly.
	struct ModelAsset {
		std::vector<MeshCPU> meshes{};
		std::vector<int> meshMaterials{}; // per meshes[i]: LOCAL index into materials, -1 = none
		std::vector<GLTFNode> nodes{};
		std::vector<GLTFSkin> skins{};
		std::vector<GLTFAnimationClip> animations{};
		std::vector<GLTFImage> images{};
		std::vector<GLTFMaterial> materials{};
		bool IsValid() const { return !nodes.empty(); }
	};
	ModelAsset ImportGLTF(const std::filesystem::path& modelPath);
}
#endif // !_GLTF_IMPORTER_HPP_