#include "RenderSystem.hpp"
#include "core/Components.hpp"
#include "engine/AssetManager.hpp"
#include "engine/Material.hpp"
#include "raylib-cpp.hpp"

namespace Long {
	void RenderSystem(entt::registry& registry, AssetManager& assets, RenderStats& stats)
	{
		stats.Reset();

		// Generic: each entity supplies a mesh (MeshFilter) and a material
		// (MeshRenderer). We draw the mesh at its WORLD transform, which the
		// TransformSystem computes (and must run before this).
		auto view = registry.view<WorldTransform, MeshFilter, MeshRenderer>();
		for (auto e : view) {
			const auto& [wt, mf, mr] = view.get<WorldTransform, MeshFilter, MeshRenderer>(e);
			if (!mr.visible || !assets.IsValidMesh(mf.meshId) || !assets.IsValidMaterial(mr.materialId)) {
				stats.culledEntities++;
				continue;
			}
			raylib::Mesh& mesh = assets.GetMesh(mf.meshId);
			BaseMaterial& material = assets.GetMaterial(mr.materialId);

			// Material binds its shader + pushes uniforms, returns the
			// raylib::Material (carrying that shader) for raylib to draw with.
			if (!assets.IsValidShader(material.GetShaderId())) {
				stats.culledEntities++;
				continue;
			}
			raylib::Shader& shader = assets.GetShader(material.GetShaderId());
			raylib::Material& rlMat = material.Apply(shader);
			mesh.Draw(rlMat, wt.matrix);

			// Each DrawMesh is one draw call; tally its geometry.
			stats.drawCalls++;
			stats.triangles += (uint32_t)mesh.GetTriangleCount();
			stats.vertices  += (uint32_t)mesh.GetVertexCount();
		}
	}
}
