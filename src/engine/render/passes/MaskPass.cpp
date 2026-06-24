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
		raylib::Shader& shader = ctx.assets->GetShader(m_material->GetShaderId());
		ctx.maskTarget->Bind();
		{
			ClearBackground(raylib::Color::Black());
			if (!ctx.selectedEntities.empty()) {
				ctx.camera->BeginMode();
				raylib::Material& rlMat = m_material->Apply(shader);
				for (entt::entity e : ctx.selectedEntities) {
					if (!reg.valid(e) || !reg.all_of<MatrixTransform, MeshFilter>(e)) {
						continue;
					}
					const auto& wt = reg.get<MatrixTransform>(e);
					const auto& mf = reg.get<MeshFilter>(e);
					if (!ctx.assets->IsValidMesh(mf.meshId)) {
						continue;
					}
					ctx.assets->GetMesh(mf.meshId).Draw(rlMat, wt.world_matrix);
				}
				ctx.camera->EndMode();
			}
		}
		ctx.renderStats.renderPassCalls++;
		ctx.maskTarget->Unbind();
	}
}