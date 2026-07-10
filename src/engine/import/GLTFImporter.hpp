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
		std::vector<int> meshIds;
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
	struct ModelAsset {
		std::vector<GLTFNode> nodes{};
		std::vector<GLTFSkin> skins{};
		std::vector<GLTFAnimationClip> animations{};
		bool IsValid() const { return !nodes.empty(); }
	};
	ModelAsset ImportGLTF(const std::filesystem::path& modelPath, AssetManager& assets);
}
#endif // !_GLTF_IMPORTER_HPP_