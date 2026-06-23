#include "TransformSystem.hpp"
#include "core/Components.hpp"
#include <raylib-cpp.hpp>

namespace Long {
	// Build the local model matrix from a Transform (scale -> rotate -> translate).
	static raylib::Matrix LocalMatrix(const Transform& t) {
		raylib::Matrix s = raylib::Matrix::Scale(t.scale.x, t.scale.y, t.scale.z);
		raylib::Matrix r = QuaternionToMatrix(t.quaternion);
		raylib::Matrix tr = raylib::Matrix::Translate(t.position.x, t.position.y, t.position.z);
		return s * r * tr;
	}

	// Recursively set world = local * parentWorld for an entity and its children.
	static void UpdateRecursive(entt::registry& reg, entt::entity e, const raylib::Matrix& parentWorld) {
		const Transform& local = reg.get<Transform>(e);
		//if auto matrix update 
		raylib::Matrix world = LocalMatrix(local) * parentWorld;
		reg.get_or_emplace<WorldTransform>(e).matrix = world;
		// Recurse into children, if any.
		if (auto* h = reg.try_get<Hierarchy>(e)) {
			for (entt::entity child : h->children) {
				if (reg.valid(child) && reg.all_of<Transform>(child)) {
					UpdateRecursive(reg, child, world);
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
				UpdateRecursive(registry, e, identity);
			}
		}
	}
}