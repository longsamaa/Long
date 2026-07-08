#include "TransformSystem.hpp"
#include "core/math/transform.hpp"
#include "helpers/draw_debug_helper.hpp" // MakeWorldBoundingBox
#include <raylib-cpp.hpp>
#include <vector>
namespace Long {
	static void UpdateRecursive(entt::registry& reg, entt::entity e, raylib::Matrix parentWorld, bool parent_change) {
		const Transform& local = reg.get<Transform>(e);
		auto& matrix_component = reg.get_or_emplace<MatrixTransform>(e);
		auto& name = reg.get<Name>(e);
		// isDirty when THIS entity's local changed, OR a parent moved (its world does).
		bool localChanged = matrix_component.builtLocalVersion != local.version;
		bool isDirty = localChanged || parent_change;
		if (isDirty) {
			matrix_component.builtLocalVersion = local.version;
			matrix_component.buildFromTransformVersion++;
			raylib::Matrix localMatrix = LocalMatrix(local);
			raylib::Matrix worldMatrix = localMatrix * parentWorld;
			matrix_component.world_matrix = worldMatrix;
			matrix_component.local_matrix = localMatrix;
			reg.emplace_or_replace<DirtyTransform>(e);
		}
		if (Hierarchy* h = reg.try_get<Hierarchy>(e)) {
			raylib::Matrix world = matrix_component.world_matrix;
			std::erase_if(h->children, [&](entt::entity child) {
				return child != entt::null && !reg.valid(child);
				});
			for (entt::entity& child : h->children) {
				if (reg.valid(child) && reg.all_of<Transform>(child)) {
					UpdateRecursive(reg, child, world, isDirty);
				}
			}
		}
	}

	void TransformSystem(entt::registry& registry) {
		static const raylib::Matrix identity = raylib::Matrix::Identity();
		std::vector<entt::entity> roots(registry.view<DirtyTransform>().begin(),
			registry.view<DirtyTransform>().end());
		for (entt::entity e : roots) {
			raylib::Matrix parentWorld = identity;
			if (const Hierarchy* h = registry.try_get<Hierarchy>(e)) {
				if (h->parent != entt::null) {
					if (const MatrixTransform* pm = registry.try_get<MatrixTransform>(h->parent)) {
						parentWorld = pm->world_matrix;
					}
				}
			}
			UpdateRecursive(registry, e, parentWorld, true);
		}
	}
}