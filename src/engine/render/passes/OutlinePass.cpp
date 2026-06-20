#include "OutlinePass.hpp"
#include "engine/AssetManager.hpp"
#include "raylib-cpp.hpp"

namespace Long {
	void OutlinePass::execute(RenderContext& ctx)
	{
		// Nothing selected -> no outline. (MaskPass already produced the mask.)
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
		raylib::Shader& shader = ctx.assets->GetShader(m_shaderId);
		raylib::TextureUnmanaged maskTex = ctx.maskTarget->GetTexture();
		int mw = (int)ctx.maskTarget->Width();
		int mh = (int)ctx.maskTarget->Height();
		raylib::Vector4 col{
			outlineColor.r / 255.0f, outlineColor.g / 255.0f,
			outlineColor.b / 255.0f, outlineColor.a / 255.0f
		};
		shader.SetValue(shader.GetLocation("size"), &outlineSize, SHADER_UNIFORM_INT);
		shader.SetValue(shader.GetLocation("width"), &mw, SHADER_UNIFORM_INT);
		shader.SetValue(shader.GetLocation("height"), &mh, SHADER_UNIFORM_INT);
		shader.SetValue(shader.GetLocation("color"), &col, SHADER_UNIFORM_VEC4);
		if (!ctx.finalTarget || !ctx.finalTarget->IsValid()) {
			return;
		}
		::Rectangle src = ctx.maskTarget->SourceRect();
		::Rectangle dst = { 0.0f, 0.0f, (float)ctx.width, (float)ctx.height };
		ctx.finalTarget->Bind();
		{
			::BeginBlendMode(BLEND_ALPHA);
			shader.BeginMode();
			::DrawTexturePro(maskTex, src, dst, { 0, 0 }, 0.0f, raylib::Color::White());
			shader.EndMode();
			::EndBlendMode();
		}
		ctx.finalTarget->Unbind();
		ctx.renderStats.renderPassCalls++;
	}
}