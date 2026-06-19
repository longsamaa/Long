#include "engine/render/passes/CompositePass.hpp"
#include "raylib-cpp.hpp"

namespace Long {

	void CompositePass::execute(RenderContext& ctx) {
		if (!ctx.sceneTarget || !ctx.sceneTarget->IsValid()) {
			return;
		}
		//Composite pass: blit scene color to the screen.
		raylib::TextureUnmanaged tex = ctx.sceneTarget->GetTexture();
		::Rectangle src = ctx.sceneTarget->SourceRect();
		::Rectangle dst = { 0.0f, 0.0f, (float)ctx.width, (float)ctx.height };
		::DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, raylib::Color::White());
	}

}
