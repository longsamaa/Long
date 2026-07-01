#include "WorldBoundSystem.hpp"
#include "core/components.hpp"
#include "helpers/draw_debug_helper.hpp"
#include "core/math/transform.hpp"
#include "engine/Logger.hpp"
namespace Long {
	void WorldBoundsSystem(entt::registry& registry, AssetManager& asset_manager) {
		auto view = registry.view<DirtyTransform, MatrixTransform, MeshFilter, Transform, Name>();
		for (entt::entity e : view) {
			const MatrixTransform& world_matrix = view.get<MatrixTransform>(e);
			WorldAABB* ptr_aabb = registry.try_get<WorldAABB>(e);
			if (!ptr_aabb) {
				MeshFilter mesh_filter = view.get<MeshFilter>(e);
				if (!asset_manager.IsValidMesh(mesh_filter.meshId)) {
					const Name& name = registry.get<Name>(e);
					Logger::TraceLog(LOG_WARNING, std::format("Valid Mesh : {}", name.value));
					continue;
				}
				raylib::Mesh& mesh = asset_manager.GetMesh(mesh_filter.meshId);
				raylib::BoundingBox box(mesh);
				raylib::BoundingBox world_box = MakeWorldBoundingBox(box, world_matrix.world_matrix);
				WorldAABB& aabb = registry.emplace<WorldAABB>(e,
					WorldAABB{ world_box.GetMin(), world_box.GetMax(),box.GetMin(),box.GetMax() });
				aabb.builtVersion = world_matrix.buildFromTransformVersion;
			}
			else {
				if (ptr_aabb->builtVersion != world_matrix.buildFromTransformVersion) {
					TransformAABB(ptr_aabb->local_min, ptr_aabb->local_max, world_matrix.world_matrix, ptr_aabb->min, ptr_aabb->max);
					ptr_aabb->builtVersion = world_matrix.buildFromTransformVersion;
				}
			}
		}
		registry.clear<DirtyTransform>();
	}
}