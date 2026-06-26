#pragma once
#include <raylib-cpp.hpp>  // Vector3 — plain POD, fine to use in components.
#include <entt/entt.hpp>
#include <string>
#include <vector>
namespace Long {
	// LOCAL transform: position/rotation/scale RELATIVE TO THE PARENT (or to the
	// world if the entity has no parent). This is what you edit.
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
		// Start at 1 so a freshly-created Transform is != MatrixTransform's
		// buildFromTransformVersion (0) -> it always gets built on the first pass,
		// even if no setter was ever called.
		uint32_t version{ 1 };
	};

	// WORLD transform: the absolute model matrix, computed by the TransformSystem
	// as  parent.world * local.  RenderSystem draws with THIS. Do not edit by hand.
	struct MatrixTransform {
		raylib::Matrix world_matrix = MatrixIdentity();
		raylib::Matrix local_matrix = MatrixIdentity();
		uint32_t buildFromTransformVersion{ 0 };
	};

	// Cached world-space AABB, recomputed only when the transform changes (see
	// TransformSystem). Lets frustum culling read bounds without re-transforming the
	// collider's 8 corners every frame. `builtVersion` mirrors the Transform version
	// the AABB was built from.
	struct WorldAABB {
		raylib::Vector3 min{ 0, 0, 0 };
		raylib::Vector3 max{ 0, 0, 0 };
		uint32_t builtVersion{ 0 };
	};

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
	};

	// Human-readable name (handy in the editor's entity list / inspector).
	struct Name {
		std::string value;
	};
	//// Tag components (empty) -- used to mark entities for systems/queries.
	//struct StaticTile {};   // part of the level geometry
	//struct Selected {};     // currently selected in the editor
} // namespace Long