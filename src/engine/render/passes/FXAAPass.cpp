#include "engine/render/GLRenderTarget.hpp"
#include "engine/render/passes/FXAAPass.hpp"
#include "engine/AssetManager.hpp"
#include "raylib-cpp.hpp"

namespace Long {
	void FXAAPass::execute(RenderContext& ctx) {
		if (!ctx.ldrTarget || !ctx.ldrTarget->IsValid() || !ctx.assets) {
			return;
		}

		// Resolve the FXAA shader once.
		if (m_shaderId == UINT32_MAX) {
			m_shaderId = ctx.assets->GetShaderId("fxaa");
			if (!ctx.assets->IsValidShader(m_shaderId)) {
				m_shaderId = UINT32_MAX;
				return;
			}
		}
		raylib::Shader& shader = ctx.assets->GetShader(m_shaderId);
		raylib::TextureUnmanaged tex = ToRaylibTexture(ctx.ldrTarget->Color());
		raylib::Vector2 res{ (float)ctx.width, (float)ctx.height };
		raylib::Rectangle src = ctx.ldrTarget->SourceRect();
		raylib::Rectangle dst = { 0.0f, 0.0f, (float)ctx.width, (float)ctx.height };
		shader.BeginMode();
		shader.SetValue(getLoc("u_resolution",shader), &res, SHADER_UNIFORM_VEC2);
		tex.Draw(src, dst, { 0, 0 }, 0.0f, raylib::Color::White());
		shader.EndMode();
	}
}