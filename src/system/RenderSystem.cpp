#include <format>
#include "RenderSystem.hpp"
#include "core/Components.hpp"
#include "engine/AssetManager.hpp"
#include "engine/Material.hpp"
#include "raylib-cpp.hpp"
#include "engine/Logger.hpp"
namespace Long {
	void RenderSystem(entt::registry& registry, AssetManager& assets, CommandQueue& queue,
		const std::vector<entt::entity>& visible, RenderStats& stats)
	{
		queue.Reserve(visible.size());
		for (entt::entity e : visible) {
			// `visible` already passed culling; still validate the entity has the
			// components and valid assets before submitting a draw.
			const MatrixTransform* wt = registry.try_get<MatrixTransform>(e);
			const MeshFilter* mf = registry.try_get<MeshFilter>(e);
			const MeshRenderer* mr = registry.try_get<MeshRenderer>(e);
			if (!wt || !mf || !mr) {
				continue;
			}
			if (!assets.IsValidMesh(mf->meshId) || !assets.IsValidMaterial(mr->materialId)) {
				Logger::TraceLog(LOG_WARNING,
					std::format("RenderSystem: Entity {} has invalid mesh or material.", entt::to_integral(e)));
				continue;
			}
			BaseMaterial& material = assets.GetMaterial(mr->materialId);
			if (!assets.IsValidShader(material.GetShaderId())) {
				Logger::TraceLog(LOG_WARNING,
					std::format("RenderSystem: Entity {} has invalid shader in material.", entt::to_integral(e)));
				continue;
			}

			const Skeleton* skeleton = nullptr;
			if (const SkinnedMeshRenderer* smr = registry.try_get<SkinnedMeshRenderer>(e)) {
				if (smr->skeleton != entt::null && registry.valid(smr->skeleton)) {
					skeleton = registry.try_get<Skeleton>(smr->skeleton);
				}
			}
			queue.Submit({
				skeleton ? raylib::Matrix::Identity() : wt->world_matrix,
				mf->meshId,   // asset id; the GL backend resolves the GPU mesh
				&material,
				!mr->visible,
				skeleton
				});
		}
	}
}