#pragma once
#include <raylib-cpp.hpp>  // Vector3 — plain POD, fine to use in components.
#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace Long {
	struct MainCamera {
		uint32_t buildFromTransformVersion{ 0 };
		uint32_t buildFromCameraParameterVersion{ 0 };
	};

	struct GameCameraParameter {
		float fov{ 45.0f };
		float near{ 0.1f };
		float far{ 50.0f };
		raylib::Vector3 target{ 0.0f,0.0f,0.0f };
		uint32_t projection{ ::CAMERA_PERSPECTIVE };
		uint32_t version{ 1 };
	};

	enum class LightType : uint32_t {
		Directional = 0,
		Point = 1,
		Spot = 2
	};

	struct LightComponent {
		LightType type;
		raylib::Vector3 direction{ 0.0f, -1.0f, 0.0f };
		raylib::Vector3 world_direction{ 0.0f, -1.0f, 0.0f };
		raylib::Color color{ 255, 255, 255, 255 };
		float intensity{ 1.0f };
		// Color temperature in Kelvin, multiplied with `color` (KelvinToRGB).
		// 6500K ~ neutral white; lower = warm/orange, higher = cool/blue.
		float temperature{ 6500.0f };
		//Spot light
		float innerAngle{ 25.0f };
		float outerAngle{ 35.0f };
		float range{ 30.0f };
		//Point light -- classic attenuation coeffs (unused by the range-based
		// falloff shader, but keep defaults so they're never garbage).
		float constant{ 1.0f };
		float linear{ 0.09f };
		float quadratic{ 0.032f };
		uint32_t version{ 1 };
		uint32_t buildFromTransformVersion{ 0 };
		bool castsShadows{ true };
	};

	struct Transform {
		raylib::Vector3 position = { 0, 0, 0 };
		raylib::Quaternion quaternion = { 0, 0, 0, 1 };
		raylib::Vector3 scale = { 1, 1, 1 };
		uint32_t version{ 1 };
	};

	//Matrix transform component
	struct MatrixTransform {
		raylib::Matrix world_matrix = MatrixIdentity();
		raylib::Matrix local_matrix = MatrixIdentity();
		uint32_t builtLocalVersion{ 0 };
		uint32_t buildFromTransformVersion{ 0 };
	};

	//World aabb component
	struct WorldAABB {
		raylib::Vector3 min{ 0, 0, 0 };
		raylib::Vector3 max{ 0, 0, 0 };
		raylib::Vector3 local_min{ 0,0,0 };
		raylib::Vector3 local_max{ 0,0,0 };
		uint32_t builtVersion{ 0 };
	};

	//Local AABB

	struct DirtyTransform {};

	// Scene-graph link. A model with many parts = one parent entity with one child
	// entity per (mesh, material). Children inherit the parent's world transform.
	struct Hierarchy {
		entt::entity parent = entt::null;
		std::vector<entt::entity> children;
	};
	// Which grid cell this entity occupies (for tile-based placement).
	//struct GridPosition {
	//    int x = 0;
	//    int z = 0;
	//};
	// Which mesh (geometry) this entity uses. Mesh lives in the AssetManager;
	// the component only references it by id. (Like Unity's MeshFilter.)
	struct MeshFilter {
		uint32_t meshId = UINT32_MAX; // invalid by default
	};

	struct MeshRenderer {
		uint32_t materialId = UINT32_MAX; // invalid -> use default material
		raylib::Color tint = raylib::Color::White();
		bool visible = true;
	};

	struct BoxCollider3D {
		raylib::BoundingBox box = raylib::BoundingBox({ -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f });
		uint32_t buildFromMatrixTransformVersion{ 0 };
	};

	struct Name {
		std::string value;
	};

	struct Skeleton {
		std::vector<entt::entity> joints;        // joint entities, in skin order
		std::vector<raylib::Matrix> inverseBind; // per joint (bind-space -> joint-local)
		std::vector<raylib::Matrix> jointMatrices; // computed: jointWorld * inverseBind
		uint32_t version{ 0 };                   // bumped by SkinningSystem on rebuild
		uint32_t builtJointVersionSum{ UINT32_MAX }; // cache: sum of joints' buildFromTransformVersion at last rebuild
	};

	struct SkinnedMeshRenderer {
		uint32_t skinIndex{ 0 };
		entt::entity skeleton = entt::null;
		uint32_t lasetSkeletonVersion = UINT32_MAX;
	};

	// One keyframe track: writes ONE property (T/R/S) of ONE entity's Transform.
	// times is ascending; vec3Keys holds translation/scale keys, quatKeys holds
	// rotation keys (only the vector matching `path` is filled).
	struct AnimationChannel {
		enum class Path : uint8_t { Translation, Rotation, Scale };
		enum class Interp : uint8_t { Linear, Step };
		entt::entity target = entt::null;
		Path path{ Path::Translation };
		Interp interp{ Interp::Linear };
		std::vector<float> times;
		std::vector<raylib::Vector3> vec3Keys;
		std::vector<raylib::Quaternion> quatKeys;
	};

	struct AnimationClip {
		std::string name;
		float duration{ 0.0f }; // seconds (max keyframe time)
		std::vector<AnimationChannel> channels;
	};

	// Unity-style Animator: owns the model's clips and plays one of them each
	// frame (AnimationSystem samples the current clip and patches Transform ->
	// DirtyTransform -> TransformSystem -> SkinningSystem). Lives on the
	// imported model's root entity. The state-machine half (controllerId,
	// currentState, parameters) is layered on top of these playback fields and
	// stays inert until an AnimatorController asset is assigned.
	struct Animator {
		std::vector<AnimationClip> clips;
		int clipIndex{ 7 };   // which clip to play, -1 = none
		float time{ 0.0f };   // seconds into the clip
		float speed{ 1.0f };
		bool loop{ true };
		bool playing{ true };
	};
} // namespace Long