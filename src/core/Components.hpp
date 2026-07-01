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
	//Game camera parameter 
	struct GameCameraParameter {
		float fov{ 45.0f };
		float near{ 0.1f };
		float far{ 50.0f };
		raylib::Vector3 target{ 0.0f,0.0f,0.0f };
		uint32_t projection{ ::CAMERA_PERSPECTIVE };
		uint32_t version{ 1 }; 
	};
	//Transform component
	struct Transform {
	public:
		const raylib::Vector3& setPos(const raylib::Vector3& _position) {
			position = _position;
			MarkDirty();
			return position;
		};
		const raylib::Quaternion& setQuaternion(const raylib::Quaternion& _quaternion) {
			quaternion = _quaternion;
			MarkDirty();
			return quaternion;
		};
		const raylib::Vector3& setScale(const raylib::Vector3& _scale) {
			scale = _scale;
			MarkDirty();
			return scale;
		};
		void MarkDirty() { ++version; }
		const raylib::Vector3& getPos() const {
			return position;
		}
		const raylib::Vector3& getScale() const {
			return scale;
		}
		const raylib::Quaternion& getQuaternion() const {
			return quaternion;
		}
		const uint32_t& getVersion() const {
			return version;
		}
	private:
		raylib::Vector3 position = { 0, 0, 0 };
		raylib::Quaternion quaternion = { 0, 0, 0, 1 };
		raylib::Vector3 scale = { 1, 1, 1 };
		uint32_t version{ 1 };
	};

	//Matrix transform component
	struct MatrixTransform {
		raylib::Matrix world_matrix = MatrixIdentity();
		raylib::Matrix local_matrix = MatrixIdentity();
		uint32_t buildFromTransformVersion{ 0 };
	};

	//World aabb component 
	struct WorldAABB {
		raylib::Vector3 min{ 0, 0, 0 };
		raylib::Vector3 max{ 0, 0, 0 };
		uint32_t builtVersion{ 0 };
	};

	// Tag: this entity's Transform changed this frame and its world matrix (and its
	// children's) must be recomputed. TransformSystem only walks tagged roots, so
	// static entities (the vast majority) are skipped entirely instead of version-
	// checked every frame. setPos/setQuaternion/setScale should add this tag.
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