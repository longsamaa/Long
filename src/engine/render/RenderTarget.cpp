#include "engine/render/RenderTarget.hpp"
#include "rlgl.h"

namespace Long {
	// Build a RenderTexture whose color attachment is a half-float RGBA buffer
	// (HDR). raylib's LoadRenderTexture only makes 8-bit targets, so we assemble
	// the framebuffer with rlgl directly. Mirrors raylib's own LoadRenderTexture
	// but swaps the color format to PIXELFORMAT_UNCOMPRESSED_R16G16B16A16.
	static ::RenderTexture2D LoadRenderTextureHDR(int w, int h) {
		::RenderTexture2D target = { 0 };
		target.id = rlLoadFramebuffer();
		if (target.id == 0) {
			TRACELOG(LOG_WARNING, "RenderTarget: failed to create HDR framebuffer");
			return target;
		}
		rlEnableFramebuffer(target.id);

		// HDR color attachment (half-float so values can exceed 1.0).
		target.texture.id = rlLoadTexture(nullptr, w, h,
			PIXELFORMAT_UNCOMPRESSED_R16G16B16A16, 1);
		target.texture.width = w;
		target.texture.height = h;
		target.texture.format = PIXELFORMAT_UNCOMPRESSED_R16G16B16A16;
		target.texture.mipmaps = 1;

		// Depth as a renderbuffer (write-only, we never sample it).
		target.depth.id = rlLoadTextureDepth(w, h, true);
		target.depth.width = w;
		target.depth.height = h;
		target.depth.format = 19; // DEPTH_COMPONENT
		target.depth.mipmaps = 1;

		rlFramebufferAttach(target.id, target.texture.id,
			RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
		rlFramebufferAttach(target.id, target.depth.id,
			RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);

		if (!rlFramebufferComplete(target.id)) {
			TRACELOG(LOG_WARNING, "RenderTarget: HDR framebuffer %u incomplete", target.id);
		}
		rlDisableFramebuffer();
		return target;
	}

	void RenderTarget::SetFormat(Format fmt) {
		if (fmt == m_format) {
			return;
		}
		m_format = fmt;
		// Force the next Resize to reallocate in the new format.
		width = 0;
		height = 0;
	}

	void RenderTarget::Resize(uint32_t newWidth, uint32_t newHeight) {
		//If size not change
		if (newWidth == width && newHeight == height) {
			return;
		}
		if (newWidth == 0 || newHeight == 0) {
			return;
		}
		width = newWidth;
		height = newHeight;
		if (m_format == Format::HDR) {
			// raylib-cpp's RenderTexture owns its handle; Unload() the old one, then
			// hand it the rlgl-built HDR framebuffer to manage.
			m_texture.Unload();
			m_texture = raylib::RenderTexture2D(LoadRenderTextureHDR((int)width, (int)height));
		}
		else {
			m_texture.Load((int)width, (int)height);
		}
	}
} // namespace Long