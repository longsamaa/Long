#include "engine/render/GLRenderTarget.hpp"
#include "BlurPass.hpp"
#include "engine/AssetManager.hpp"
namespace Long {
	void BlurPass::execute(RenderContext& ctx)
	{
		if (!ctx.brightTarget || !ctx.brightTarget->IsValid() ||
			!ctx.blurTarget || !ctx.assets) {
			return;
		}
		if (m_shaderId == UINT32_MAX) {
			m_shaderId = ctx.assets->GetShaderId("blur");
			if (!ctx.assets->IsValidShader(m_shaderId)) {
				m_shaderId = UINT32_MAX;
				return;
			}
		}
		const uint32_t bw = ctx.width / down_scale;
		const uint32_t bh = ctx.height / down_scale;
		ctx.blurTarget->Resize(bw, bh);
		m_blurTarget.Resize(bw, bh);

		raylib::Shader& shader = ctx.assets->GetShader(m_shaderId);
		raylib::Rectangle dst = { 0.0f, 0.0f, (float)bw, (float)bh };
		{
			raylib::Vector2 dir(1.0f / (float)bw, 0.0f);
			raylib::TextureUnmanaged srcTex = ToRaylibTexture(ctx.brightTarget->Color());
			raylib::Rectangle src = ctx.brightTarget->SourceRect();
			m_blurTarget.Bind();
			raylib::Color::Black().ClearBackground(); 
			shader.BeginMode();
			shader.SetValue(getLoc("u_radius",shader), &u_radius, SHADER_UNIFORM_INT);
			shader.SetValue(getLoc("u_texelDir",shader), &dir, SHADER_UNIFORM_VEC2);
			srcTex.Draw(src, dst, { 0, 0 }, 0.0f, raylib::Color::White());
			shader.EndMode();
			m_blurTarget.Unbind();
		}
		{
			raylib::Vector2 dir(0.0f, 1.0f / (float)bh);
			raylib::Rectangle src = m_blurTarget.SourceRect();
			ctx.blurTarget->Bind();
			raylib::Color::Black().ClearBackground();
			shader.BeginMode();
			shader.SetValue(getLoc("u_radius",shader), &u_radius, SHADER_UNIFORM_INT);
			shader.SetValue(getLoc("u_texelDir",shader), &dir, SHADER_UNIFORM_VEC2);
			ToRaylibTexture(m_blurTarget.Color()).Draw(src, dst, { 0, 0 }, 0.0f, raylib::Color::White());
			shader.EndMode();
			ctx.blurTarget->Unbind();
		}
	}
}