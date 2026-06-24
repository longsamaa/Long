#include "TransformSystem.hpp"
#include "core/Components.hpp"
#include <raylib-cpp.hpp>

namespace Long {
	// Build the local model matrix from a Transform (scale -> rotate -> translate).
	static raylib::Matrix LocalMatrix(const Transform& t) {
		const raylib::Vector3& scale = t.getScale(); 
		const raylib::Quaternion& quaternion = t.getQuaternion();
		const raylib::Vector3& position = t.getPos();
		raylib::Matrix s = raylib::Matrix::Scale(scale.x, scale.y, scale.z);
		raylib::Matrix r = QuaternionToMatrix(quaternion);
		raylib::Matrix tr = raylib::Matrix::Translate(position.x, position.y, position.z);
		return s * r * tr;
	}

	// Recursively set world = local * parentWorld for an entity and its children.
	static void UpdateRecursive(entt::registry& reg, entt::entity e, const raylib::Matrix& parentWorld, bool parent_change) {
		const Transform& local = reg.get<Transform>(e);
		//if auto matrix update 		
		auto& matrix_component = reg.get_or_emplace<MatrixTransform>(e);
		bool isDirty = matrix_component.buildFromTransformVersion != local.getVersion() || parent_change; 
		if (isDirty) {
			//update version 
			matrix_component.buildFromTransformVersion = local.getVersion(); 
			raylib::Matrix localMatrix = LocalMatrix(local);
			raylib::Matrix worldMatrix = localMatrix * parentWorld;
			matrix_component.world_matrix = worldMatrix;
			matrix_component.local_matrix = localMatrix;
		}
		// Recurse into children, if any.
		if (auto* h = reg.try_get<Hierarchy>(e)) {
			for (entt::entity child : h->children) {
				if (reg.valid(child) && reg.all_of<Transform>(child)) {
					UpdateRecursive(reg, child, matrix_component.world_matrix, isDirty);
				}
			}
		}
	}

	void TransformSystem(entt::registry& registry) {
		const raylib::Matrix identity = raylib::Matrix::Identity();
		auto view = registry.view<Transform>();
		for (entt::entity e : view) {
			const Hierarchy* h = registry.try_get<Hierarchy>(e);
			bool isRoot = (h == nullptr) || (h->parent == entt::null);
			if (isRoot) {
				UpdateRecursive(registry, e, identity,false);
			}
		}
	}
}