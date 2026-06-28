#include <format>
#include "RenderSystem.hpp"
#include "core/Components.hpp"
#include "engine/AssetManager.hpp"
#include "engine/Material.hpp"
#include "raylib-cpp.hpp"
#include "engine/Logger.hpp"
#include "engine/visibility/FrustumCulling.hpp"
#include "helpers/draw_debug_helper.hpp"
namespace Long {
	void RenderSystem(entt::registry& registry, AssetManager& assets, CommandQueue& queue,
		const FrustumCulling* frustum, RenderStats& stats)
	{
		auto view = registry.view<MatrixTransform, MeshFilter, MeshRenderer>();
		queue.Reserve(view.size_hint());
		for (auto e : view) {
			const auto& [wt, mf, mr] = view.get<MatrixTransform, MeshFilter, MeshRenderer>(e);
			if (!assets.IsValidMesh(mf.meshId) || !assets.IsValidMaterial(mr.materialId)) {
				Logger::TraceLog(LOG_WARNING,
					std::format("RenderSystem: Entity {} has invalid mesh or material.", entt::to_integral(e)));
				continue;
			}
			if (frustum) {
				if (const WorldAABB* aabb = registry.try_get<WorldAABB>(e)) {
					if (!frustum->isVisible(raylib::BoundingBox(aabb->min, aabb->max))) {
						stats.culledEntities++;
						continue;
					}
				}
			}
			raylib::Mesh& mesh = assets.GetMesh(mf.meshId);
			BaseMaterial& material = assets.GetMaterial(mr.materialId);
			if (!assets.IsValidShader(material.GetShaderId())) {
				Logger::TraceLog(LOG_WARNING,
					std::format("RenderSystem: Entity {} has invalid shader in material.", entt::to_integral(e)));
				continue;
			}
			//Push to queue instead of drawing directly
			queue.Submit({
				wt.world_matrix,
				&mesh,
				&material,
				!mr.visible
				});
		}
	}
}