#include "engine/render/GLRenderTarget.hpp"
#include "BloomPass.hpp"
#include "engine/AssetManager.hpp"
#include "engine/render/RenderContext.hpp"
#include "engine/render/RenderState.hpp"
#include "raylib-cpp.hpp"
#include "rlgl.h"

namespace Long {
	// (Re)allocate the mip chain when the base (brightTarget) size changes. Each mip
	// is half the previous one, clamped to a minimum of 1 so we never hit 0.
	void BloomPass::ensureMips(uint32_t baseW, uint32_t baseH) {
		if (baseW == m_baseW && baseH == m_baseH) {
			return;
		}
		m_baseW = baseW;
		m_baseH = baseH;
		uint32_t w = baseW, h = baseH;
		for (int i = 0; i < kMipCount; ++i) {
			w = (w > 1) ? w / 2 : 1;
			h = (h > 1) ? h / 2 : 1;
			m_mips[i].SetFormat(RenderTarget::Format::HDR);
			m_mips[i].Resize(w, h);
		}
	}

	void BloomPass::execute(RenderContext& ctx) {
		if (!ctx.brightTarget || !ctx.brightTarget->IsValid() ||
			!ctx.blurTarget || !ctx.assets) {
			return;
		}
		if (m_downShaderId == UINT32_MAX) {
			m_downShaderId = ctx.assets->GetShaderId("bloom_downsample");
			m_upShaderId = ctx.assets->GetShaderId("bloom_upsample");
			if (!ctx.assets->IsValidShader(m_downShaderId) ||
				!ctx.assets->IsValidShader(m_upShaderId)) {
				m_downShaderId = UINT32_MAX;
				return;
			}
		}
		ensureMips(ctx.brightTarget->Width(), ctx.brightTarget->Height());

		raylib::Shader& down = ctx.assets->GetShader(m_downShaderId);
		raylib::Shader& up = ctx.assets->GetShader(m_upShaderId);

		auto drawFull = [](RenderTarget& srcT, RenderTarget& dst) {
			raylib::TextureUnmanaged tex = ToRaylibTexture(srcT.Color());
			raylib::Rectangle src = { 0, 0, (float)srcT.Width(), (float)srcT.Height() };
			raylib::Rectangle d = { 0, 0, (float)dst.Width(), (float)dst.Height() };
			tex.Draw(src, d, { 0, 0 }, 0.0f, raylib::Color::White());
			};
		for (int i = 0; i < kMipCount; ++i) {
			RenderTarget& srcT = (i == 0) ? *ctx.brightTarget : m_mips[i - 1];
			RenderTarget& dstT = m_mips[i];
			raylib::Vector2 srcTexel{ 1.0f / (float)srcT.Width(), 1.0f / (float)srcT.Height() };
			int firstPass = (i == 0) ? 1 : 0; // Karis on bright -> mip0 only
			dstT.Bind();
			raylib::Color::Black().ClearBackground();
			down.BeginMode();
			down.SetValue(getLoc("u_srcTexel", down), &srcTexel, SHADER_UNIFORM_VEC2);
			down.SetValue(getLoc("u_firstPass", down), &firstPass, SHADER_UNIFORM_INT);
			drawFull(srcT, dstT);
			down.EndMode();
			dstT.Unbind();
		}
		for (int i = kMipCount - 1; i > 0; --i) {
			RenderTarget& srcT = m_mips[i];       // smaller
			RenderTarget& dstT = m_mips[i - 1];   // larger
			raylib::Vector2 srcTexel{ 1.0f / (float)srcT.Width(), 1.0f / (float)srcT.Height() };
			dstT.Bind();
			{
				ScopedBlend blend(RL_BLEND_ADDITIVE); // auto-restored at block end
				up.BeginMode();
				up.SetValue(getLoc("u_srcTexel", up), &srcTexel, SHADER_UNIFORM_VEC2);
				up.SetValue(getLoc("u_radius", up), &upsampleRadius, SHADER_UNIFORM_FLOAT);
				drawFull(srcT, dstT);
				up.EndMode();
			}
			dstT.Unbind();
		}
		ctx.blurTarget->Resize(m_mips[0].Width(), m_mips[0].Height());
		ctx.blurTarget->Bind();
		raylib::Color::Black().ClearBackground();
		drawFull(m_mips[0], *ctx.blurTarget);
		ctx.blurTarget->Unbind();
	}
}