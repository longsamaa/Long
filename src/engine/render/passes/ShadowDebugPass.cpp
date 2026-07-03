#include "engine/render/passes/ShadowDebugPass.hpp"
#include "engine/AssetManager.hpp"
#include "raylib-cpp.hpp"
#include "rlgl.h"

namespace Long {
	void ShadowDebugPass::execute(RenderContext& ctx) {
		if (!ctx.assets || !ctx.lights || ctx.lights->shadowMapTexId == 0) {
			return;
		}
		uint32_t shaderId = ctx.assets->GetShaderId("depth_debug");
		if (!ctx.assets->IsValidShader(shaderId)) {
			return;
		}

		// Wrap the raw GL depth texture id in a Texture so DrawTexturePro can use
		// it. The depth map is square (m_resolution); we draw it scaled into a
		// corner. format/mipmaps are cosmetic here (raylib only needs id + size).
		int res = (int)ctx.lights->shadowMapSize;
		if (res <= 0) res = 1;
		::Texture2D depthTex{};
		depthTex.id = ctx.lights->shadowMapTexId;
		depthTex.width = res;
		depthTex.height = res;
		depthTex.mipmaps = 1;
		depthTex.format = PIXELFORMAT_UNCOMPRESSED_R32; // single-channel depth

		const float size = 256.0f;
		const float margin = 10.0f;
		float sx = (float)::GetScreenWidth() - size - margin;
		float sy = margin;

		raylib::Shader& shader = ctx.assets->GetShader(shaderId);
		shader.BeginMode();
		::Rectangle src{ 0, 0, (float)res, (float)res };
		::Rectangle dst{ sx, sy, size, size };
		DrawTexturePro(depthTex, src, dst, { 0, 0 }, 0.0f, WHITE);
		shader.EndMode();
	}
}
