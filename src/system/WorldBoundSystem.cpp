#include "WorldBoundSystem.hpp"
#include "core/components.hpp"
#include "helpers/draw_debug_helper.hpp"
#include "engine/Logger.hpp"
namespace Long {
	void WorldBoundsSystem(entt::registry& registry, AssetManager& asset_manager) {
		auto view = registry.view<MeshFilter, Transform, Name>();
		for (entt::entity e : view) {
			//lay AABB component neu ko co thi create
			WorldAABB& aabb = registry.get_or_emplace<WorldAABB>(e);
			const MatrixTransform& world_matrix = registry.get<MatrixTransform>(e);
			if (aabb.builtVersion != world_matrix.buildFromTransformVersion) {
				//tao aabb moi
				const MeshFilter& mesh_filter = registry.get<MeshFilter>(e);
				if (!asset_manager.IsValidMesh(mesh_filter.meshId))
				{
					const Name& name = registry.get<Name>(e);
					Logger::TraceLog(LOG_WARNING, std::format("Valid Mesh : {}", name.value));
					continue;
				}
				raylib::Mesh& mesh = asset_manager.GetMesh(mesh_filter.meshId);
				raylib::BoundingBox box(mesh);
				raylib::BoundingBox world_box = MakeWorldBoundingBox(box, world_matrix.world_matrix);
				aabb.min = world_box.GetMin();
				aabb.max = world_box.GetMax();
				aabb.builtVersion = world_matrix.buildFromTransformVersion;
			}
		}
	}
}