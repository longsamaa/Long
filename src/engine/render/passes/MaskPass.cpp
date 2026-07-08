#include "engine/render/passes/MaskPass.hpp"
#include "core/Components.hpp"
#include "engine/AssetManager.hpp"
namespace Long {
	void MaskPass::execute(RenderContext& ctx) {
		if (!ctx.maskTarget || !ctx.registry || !ctx.assets || !ctx.camera) {
			return;
		}
		ctx.maskTarget->Resize(ctx.width, ctx.height);
		if (!ctx.maskTarget->IsValid()) {
			return;
		}
		if (!m_material) {
			uint32_t shaderId = ctx.assets->GetShaderId("mask");
			if (!ctx.assets->IsValidShader(shaderId)) {
				return;
			}
			m_material = std::make_unique<MaskMaterial>(shaderId, raylib::Color::White());
		}
		auto& reg = *ctx.registry;
		ctx.maskTarget->Bind();
		{
			raylib::Color::Black().ClearBackground();
			if (!ctx.selectedEntities.empty()) {
				ctx.camera->BeginMode();
				for (entt::entity e : ctx.selectedEntities) {
					if (!reg.valid(e) || !reg.all_of<MatrixTransform, MeshFilter>(e)) {
						continue;
					}
					const auto& wt = reg.get<MatrixTransform>(e);
					const auto& mf = reg.get<MeshFilter>(e);
					if (!ctx.assets->IsValidMesh(mf.meshId)) {
						continue;
					}
					// CPU material -> backend resolves shader + uniforms + draw.
					ctx.glRenderer->DrawMeshImmediate(*ctx.assets, mf.meshId,
						*m_material, wt.world_matrix);
				}
				ctx.camera->EndMode();
			}
		}
		ctx.maskTarget->Unbind();
	}
}