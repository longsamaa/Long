#include "engine/render/passes/CompositePass.hpp"
#include "raylib-cpp.hpp"

namespace Long {

	void CompositePass::execute(RenderContext& ctx) {
		if (!ctx.sceneTarget || !ctx.sceneTarget->IsValid() || !ctx.finalTarget) {
			return;
		}
		
		ctx.finalTarget->Resize(ctx.width, ctx.height);
		if (!ctx.finalTarget->IsValid()) {
			return;
		}

		raylib::TextureUnmanaged tex = ctx.sceneTarget->GetTexture();
		::Rectangle src = ctx.sceneTarget->SourceRect();
		::Rectangle dst = { 0.0f, 0.0f, (float)ctx.width, (float)ctx.height };

		ctx.finalTarget->Bind();
		::DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, raylib::Color::White());
		ctx.finalTarget->Unbind();
		ctx.renderStats.renderPassCalls++;
	}

}
