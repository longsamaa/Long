#include "engine/render/passes/ShadowDebugPass.hpp"
#include "engine/AssetManager.hpp"
#include "raylib-cpp.hpp"
#include "rlgl.h"

namespace Long {
	void ShadowDebugPass::execute(RenderContext& ctx) {
		if (!ctx.assets || !ctx.lights || ctx.lights->shadowCount == 0) {
			return;
		}
		uint32_t shaderId = ctx.assets->GetShaderId("depth_debug");
		if (!ctx.assets->IsValidShader(shaderId)) {
			return;
		}

		const float size = 256.0f;
		const float margin = 10.0f;
		raylib::Shader& shader = ctx.assets->GetShader(shaderId);
		shader.BeginMode();
		// One tile per rendered shadow map, stacked down the right edge.
		for (uint32_t k = 0; k < ctx.lights->shadowCount; ++k) {
			const ShadowCaster& sc = ctx.lights->shadows[k];
			if (sc.depthTexId == 0) {
				continue;
			}
			// Wrap the raw GL depth texture id so DrawTexturePro can use it; the
			// exact size only matters for the source rect, maps are square.
			::Texture2D depthTex{};
			depthTex.id = sc.depthTexId;
			depthTex.width = 2048;
			depthTex.height = 2048;
			depthTex.mipmaps = 1;
			depthTex.format = PIXELFORMAT_UNCOMPRESSED_R32; // single-channel depth
			float sx = (float)::GetScreenWidth() - size - margin;
			float sy = margin + (float)k * (size + margin);
			::Rectangle src{ 0, 0, (float)depthTex.width, (float)depthTex.height };
			::Rectangle dst{ sx, sy, size, size };
			DrawTexturePro(depthTex, src, dst, { 0, 0 }, 0.0f, WHITE);
		}
		shader.EndMode();
	}
}