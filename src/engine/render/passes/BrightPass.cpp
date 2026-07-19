#include "engine/render/GLRenderTarget.hpp"
#include "BrightPass.hpp"
#include "engine/AssetManager.hpp"
#include "engine/Environment.hpp"
#include "raylib-cpp.hpp"

namespace Long {
	void BrightPass::execute(RenderContext& ctx)
	{
		if (!ctx.brightTarget || !ctx.sceneTarget || !ctx.assets) {
			return;
		}
		if (!ctx.sceneTarget->IsValid()) {
			return;
		}
		const uint32_t bw = ctx.width / down_scale;
		const uint32_t bh = ctx.height / down_scale;
		ctx.brightTarget->Resize(bw, bh);
		if (m_shaderId == UINT32_MAX) {
			m_shaderId = ctx.assets->GetShaderId("brightpass");
			if (!ctx.assets->IsValidShader(m_shaderId)) {
				m_shaderId = UINT32_MAX;
				return;
			}
		}
		raylib::Shader& shader = ctx.assets->GetShader(m_shaderId);
		// Live-tunable settings come from the Environment when present.
		if (ctx.environment) {
			u_threshold = ctx.environment->bloomThreshold;
			u_softKnee = ctx.environment->bloomSoftKnee;
			u_clampMax = ctx.environment->bloomClampMax;
		}
		raylib::TextureUnmanaged sceneTex = ToRaylibTexture(ctx.sceneTarget->Color());
		raylib::Rectangle src = ctx.sceneTarget->SourceRect(); // negative height: GL flip
		raylib::Rectangle dst = { 0.0f, 0.0f, (float)bw, (float)bh };
		ctx.brightTarget->Bind();
		{
			shader.BeginMode();
			shader.SetValue(getLoc("u_threshold",shader), &u_threshold, SHADER_UNIFORM_FLOAT);
			shader.SetValue(getLoc("u_softKnee",shader), &u_softKnee, SHADER_UNIFORM_FLOAT);
			shader.SetValue(getLoc("u_clampMax",shader), &u_clampMax, SHADER_UNIFORM_FLOAT);
			sceneTex.Draw(src, dst, { 0, 0 }, 0.0f, raylib::Color::White());
			shader.EndMode();
		}
		ctx.brightTarget->Unbind();
	}
}