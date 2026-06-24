#include <format>
#include "RenderSystem.hpp"
#include "core/Components.hpp"
#include "engine/AssetManager.hpp"
#include "engine/Material.hpp"
#include "raylib-cpp.hpp"
#include "engine/Logger.hpp"
namespace Long {
	void RenderSystem(entt::registry& registry, AssetManager& assets, CommandQueue& queue)
	{
		//stats.Reset();
		auto view = registry.view<MatrixTransform, MeshFilter, MeshRenderer>();
		// Pre-grow the command buffer once so the per-entity Submit() loop below
		// doesn't trigger repeated vector reallocations.
		queue.Reserve(view.size_hint());
		for (auto e : view) {
			const auto& [wt, mf, mr] = view.get<MatrixTransform, MeshFilter, MeshRenderer>(e);
			if (!assets.IsValidMesh(mf.meshId) || !assets.IsValidMaterial(mr.materialId)) {
				Logger::TraceLog(LOG_WARNING,
					std::format("RenderSystem: Entity {} has invalid mesh or material.", entt::to_integral(e)));
				continue;
			}
			raylib::Mesh& mesh = assets.GetMesh(mf.meshId);
			BaseMaterial& material = assets.GetMaterial(mr.materialId);
			if (!assets.IsValidShader(material.GetShaderId())) {
				//stats.culledEntities++;
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