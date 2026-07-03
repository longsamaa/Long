#include "engine/render/RenderTarget.hpp"
#include "rlgl.h"

namespace Long {
	static raylib::RenderTexture2D LoadRenderTextureHDR(int w, int h) {
		raylib::RenderTexture2D target(::RenderTexture2D(0));
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

	static raylib::RenderTexture2D LoadDepthTexture(int w, int h)
	{
		raylib::RenderTexture2D target(::RenderTexture2D(0));

		target.id = rlLoadFramebuffer();
		if (target.id == 0)
		{
			TRACELOG(LOG_WARNING, "RenderTarget: failed to create depth framebuffer");
			return target;
		}

		rlEnableFramebuffer(target.id);

		// No color attachment, but BeginTextureMode sets the viewport from
		// texture.width/height -- leave them 0 and NOTHING rasterizes. Set the
		// size even though texture.id stays 0.
		target.texture.width = w;
		target.texture.height = h;

		// Create depth texture (not renderbuffer!)
		target.depth.id = rlLoadTextureDepth(w, h, false);
		target.depth.width = w;
		target.depth.height = h;
		target.depth.format = 19;      // DEPTH_COMPONENT
		target.depth.mipmaps = 1;

		rlFramebufferAttach(target.id,
			target.depth.id,
			RL_ATTACHMENT_DEPTH,
			RL_ATTACHMENT_TEXTURE2D,
			0);

		rlActiveDrawBuffers(0);
		if (!rlFramebufferComplete(target.id))
		{
			TRACELOG(LOG_WARNING,
				"RenderTarget: depth framebuffer %u incomplete",
				target.id);
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
			m_texture.Unload();
			m_texture = raylib::RenderTexture2D(LoadRenderTextureHDR((int)width, (int)height));
		}
		else if (m_format == Format::DEPTH) {
			m_texture.Unload();
			m_texture = raylib::RenderTexture2D(LoadDepthTexture((int)width, (int)height));
		}
		else {
			m_texture.Load((int)width, (int)height);
		}
		if (m_format == Format::DEPTH) {
			// No color texture; clamp the DEPTH texture so shadow samples outside
			// the light frustum read edge depth instead of wrapping. NEAREST filter:
			// the shader does bilinear PCF itself (it interpolates comparison
			// RESULTS, so it must read raw per-texel depth).
			rlTextureParameters(m_texture.depth.id, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_CLAMP);
			rlTextureParameters(m_texture.depth.id, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);
			rlTextureParameters(m_texture.depth.id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_NEAREST);
			rlTextureParameters(m_texture.depth.id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_NEAREST);
		}
		else {
			SetTextureWrap(m_texture.texture, TEXTURE_WRAP_CLAMP);
			SetTextureFilter(m_texture.texture, TEXTURE_FILTER_BILINEAR);
		}
	}
} // namespace Long