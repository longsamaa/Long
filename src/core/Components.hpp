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
		raylib::Color color{ 255, 255, 255, 255 };
		float intensity{ 1.0f };
		uint32_t version{ 1 };
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
		// Snapshot of the source Transform::version the last time we rebuilt -- lets
		// TransformSystem detect a LOCAL change.
		uint32_t builtLocalVersion{ 0 };
		// Monotonic counter bumped whenever world_matrix is recomputed (local OR parent
		// change). WorldBoundsSystem compares its aabb.builtVersion against this.
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

	// How this entity is shaded. Material (shader + params) lives in the
	// AssetManager; referenced by id so several entities can share/override it.
	// (Like Unity's MeshRenderer.)
	struct MeshRenderer {
		uint32_t materialId = UINT32_MAX; // invalid -> use default material
		raylib::Color tint = raylib::Color::White();
		bool visible = true;
	};

	struct BoxCollider3D {
		raylib::BoundingBox box = raylib::BoundingBox({ -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f });
		uint32_t buildFromMatrixTransformVersion{ 0 };
	};

	// Human-readable name (handy in the editor's entity list / inspector).
	struct Name {
		std::string value;
	};
	//// Tag components (empty) -- used to mark entities for systems/queries.
	//struct StaticTile {};   // part of the level geometry
	//struct Selected {};     // currently selected in the editor
} // namespace Long