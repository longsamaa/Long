#include "TonemapPass.hpp"
#include "engine/AssetManager.hpp"
#include "raylib-cpp.hpp"

namespace Long {
	void TonemapPass::execute(RenderContext& ctx)
	{
		if (!ctx.finalTarget || !ctx.finalTarget->IsValid() || !ctx.assets) {
			return;
		}
		if (m_shaderId == UINT32_MAX) {
			m_shaderId = ctx.assets->GetShaderId("tonemap_fxaa");
			if (!ctx.assets->IsValidShader(m_shaderId)) {
				m_shaderId = UINT32_MAX;
				return;
			}
		}
		raylib::Shader& shader = ctx.assets->GetShader(m_shaderId);
		raylib::TextureUnmanaged hdrTex = ctx.finalTarget->GetTexture();
		raylib::Rectangle src = ctx.finalTarget->SourceRect(); 
		raylib::Rectangle dst = { 0.0f, 0.0f, (float)ctx.width, (float)ctx.height };
		raylib::Vector2 res{ (float)ctx.width, (float)ctx.height };
		int fxaa = fxaaEnabled ? 1 : 0;
		shader.BeginMode();
		shader.SetValue(getLoc("u_exposure", shader), &u_exposure, SHADER_UNIFORM_FLOAT);
		shader.SetValue(getLoc("u_resolution", shader), &res, SHADER_UNIFORM_VEC2);
		shader.SetValue(getLoc("u_fxaaEnabled", shader), &fxaa, SHADER_UNIFORM_INT);
		hdrTex.Draw(src, dst, { 0, 0 }, 0.0f, raylib::Color::White());
		shader.EndMode();
	}
}
