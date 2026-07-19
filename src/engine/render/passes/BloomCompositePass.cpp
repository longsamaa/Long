#include "engine/render/GLRenderTarget.hpp"
#include "BloomCompositePass.hpp"
#include "engine/AssetManager.hpp"
#include "engine/Environment.hpp"
#include "raylib-cpp.hpp"

namespace Long {
	void BloomCompositePass::execute(RenderContext& ctx)
	{
		if (!ctx.sceneTarget || !ctx.sceneTarget->IsValid() ||
			!ctx.blurTarget || !ctx.blurTarget->IsValid() ||
			!ctx.finalTarget || !ctx.assets) {
			return;
		}
		ctx.finalTarget->Resize(ctx.width, ctx.height);
		if (!ctx.finalTarget->IsValid()) {
			return;
		}

		if (m_shaderId == UINT32_MAX) {
			m_shaderId = ctx.assets->GetShaderId("bloom_composite");
			if (!ctx.assets->IsValidShader(m_shaderId)) {
				m_shaderId = UINT32_MAX;
				return;
			}
		}
		raylib::Shader& shader = ctx.assets->GetShader(m_shaderId);
		// Live-tunable strength from the Environment when present.
		if (ctx.environment) {
			u_bloomStrength = ctx.environment->bloomStrength;
		}
		raylib::TextureUnmanaged bloomTex = ToRaylibTexture(ctx.blurTarget->Color());
		raylib::TextureUnmanaged sceneTex = ToRaylibTexture(ctx.sceneTarget->Color());
		raylib::Rectangle src = ctx.sceneTarget->SourceRect(); 
		raylib::Rectangle dst = { 0.0f, 0.0f, (float)ctx.width, (float)ctx.height };

		ctx.finalTarget->Bind();
		{
			shader.BeginMode();
			raylib::Color::Black().ClearBackground(); 
			shader.SetValue(getLoc("u_bloomStrength",shader), &u_bloomStrength, SHADER_UNIFORM_FLOAT);
			SetShaderValueTexture(shader, getLoc("u_bloom",shader), bloomTex);
			sceneTex.Draw(src, dst, { 0, 0 }, 0.0f, raylib::Color::White());
			shader.EndMode();
		}
		ctx.finalTarget->Unbind();
		ctx.renderStats.renderPassCalls++;
	}
}
