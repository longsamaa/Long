#pragma once
#include <raylib-cpp.hpp>  // Vector3 — plain POD, fine to use in components.
#include <entt/entt.hpp>
#include <string>
#include <vector>
namespace Long {
	// LOCAL transform: position/rotation/scale RELATIVE TO THE PARENT (or to the
	// world if the entity has no parent). This is what you edit.
	struct Transform {
		raylib::Vector3 position = { 0, 0, 0 };
		raylib::Quaternion quaternion = { 0, 0, 0, 1 }; // identity rotation
		raylib::Vector3 scale = { 1, 1, 1 };

		// Bumped whenever this transform changes. TransformSystem compares it with
		// WorldTransform::builtVersion and only recomputes the world matrix when they
		// differ -- so static objects cost nothing per frame. Call MarkDirty() (or
		// just ++version) after editing position/quaternion/scale. Starts at 1 so the
		// world matrix (builtVersion 0) is computed once on the first frame.
		uint32_t version = 1;
		void MarkDirty() { ++version; }
	};

	// WORLD transform: the absolute model matrix, computed by the TransformSystem
	// as  parent.world * local.  RenderSystem draws with THIS. Do not edit by hand.
	struct WorldTransform {
		raylib::Matrix matrix = MatrixIdentity();
		// Value of Transform::version when `matrix` was last computed.
		uint32_t builtVersion = 0;
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