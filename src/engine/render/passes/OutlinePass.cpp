#include "engine/render/GLRenderTarget.hpp"
#include "OutlinePass.hpp"
#include "engine/AssetManager.hpp"
#include "raylib-cpp.hpp"

namespace Long {
	void OutlinePass::execute(RenderContext& ctx)
	{
		if (!ctx.maskTarget || !ctx.maskTarget->IsValid() ||
			!ctx.assets || ctx.selectedEntities.empty()) {
			return;
		}
		if (m_shaderId == UINT32_MAX) {
			m_shaderId = ctx.assets->GetShaderId("outline");
			if (!ctx.assets->IsValidShader(m_shaderId)) {
				m_shaderId = UINT32_MAX;
				return;
			}
		}
		if (!ctx.finalTarget || !ctx.finalTarget->IsValid()) {
			return;
		}
		const int scale = (downscale < 1) ? 1 : downscale;
		ctx.maskTarget->Resize(ctx.width, ctx.height);
		const uint32_t lowW = (ctx.width + scale - 1) / scale;
		const uint32_t lowH = (ctx.height + scale - 1) / scale;
		m_tempTarget.Resize(lowW, lowH);
		raylib::Shader& shader = ctx.assets->GetShader(m_shaderId);
		raylib::Vector4 col{
			outlineColor.r / 255.0f, outlineColor.g / 255.0f,
			outlineColor.b / 255.0f, outlineColor.a / 255.0f
		};
		const int locSize = getLoc("size", shader); 
		const int locColor = getLoc("color",shader);
		const int locDir = getLoc("direction",shader);
		const int locFinal = getLoc("isFinal",shader);
		const int locW = getLoc("width",shader);
		const int locH = getLoc("height",shader);
		{
			int sw = (int)ctx.width, sh = (int)ctx.height;
			raylib::Vector2 dir{ 1.0f, 0.0f };
			int isFinal = 0;
			raylib::TextureUnmanaged maskTex = ToRaylibTexture(ctx.maskTarget->Color());
			::Rectangle dstLow = { 0.0f, 0.0f, (float)lowW, (float)lowH };
			m_tempTarget.Bind();
			shader.BeginMode();
			shader.SetValue(locSize, &outlineSize, SHADER_UNIFORM_INT);
			shader.SetValue(locColor, &col, SHADER_UNIFORM_VEC4);
			shader.SetValue(locW, &sw, SHADER_UNIFORM_INT);
			shader.SetValue(locH, &sh, SHADER_UNIFORM_INT);
			shader.SetValue(locDir, &dir, SHADER_UNIFORM_VEC2);
			shader.SetValue(locFinal, &isFinal, SHADER_UNIFORM_INT);
			maskTex.Draw(ctx.maskTarget->SourceRect(), dstLow, { 0, 0 }, 0.0f, raylib::Color::White());
			shader.EndMode();
			m_tempTarget.Unbind();
		}
		{
			int sw = (int)lowW, sh = (int)lowH;
			raylib::Vector2 dir{ 0.0f, 1.0f };
			int isFinal = 1;
			raylib::TextureUnmanaged tempTex = ToRaylibTexture(m_tempTarget.Color());
			::Rectangle dstFull = { 0.0f, 0.0f, (float)ctx.width, (float)ctx.height };
			shader.BeginMode();
			shader.SetValue(locSize, &outlineSize, SHADER_UNIFORM_INT);
			shader.SetValue(locColor, &col, SHADER_UNIFORM_VEC4);
			shader.SetValue(locW, &sw, SHADER_UNIFORM_INT);
			shader.SetValue(locH, &sh, SHADER_UNIFORM_INT);
			shader.SetValue(locDir, &dir, SHADER_UNIFORM_VEC2);
			shader.SetValue(locFinal, &isFinal, SHADER_UNIFORM_INT);
			tempTex.Draw(m_tempTarget.SourceRect(), dstFull, { 0, 0 }, 0.0f, raylib::Color::White());
			shader.EndMode();
		}
	}
}
