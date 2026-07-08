#include "engine/render/GLRenderTarget.hpp"
#include "rlgl.h"
// raylib already links + loads GLAD (in its rlgl translation unit). We include
// the header WITHOUT GLAD_GL_IMPLEMENTATION to get the GL enums/prototypes only;
// the function pointers are the ones raylib already resolved. Needed because
// rlgl has no API to allocate a DEPTH cubemap (rlLoadTextureCubemap rejects
// depth formats), so the point-light shadow cube is built with raw GL here.
#include "external/glad.h"

namespace Long {
	static raylib::RenderTexture2D LoadRenderTextureHDR(int w, int h) {
		raylib::RenderTexture2D target(::RenderTexture2D(0));
		target.id = rlLoadFramebuffer();
		if (target.id == 0) {
			TRACELOG(LOG_WARNING, "GLRenderTarget: failed to create HDR framebuffer");
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
			TRACELOG(LOG_WARNING, "GLRenderTarget: HDR framebuffer %u incomplete", target.id);
		}
		rlDisableFramebuffer();
		return target;
	}

	// Depth CUBEMAP framebuffer for omnidirectional (point-light) shadows. Six
	// faces of GL_DEPTH_COMPONENT; the scene is rendered once per face from the
	// light's position, and the shader samples the cube by the fragment->light
	// direction. `size` is the edge length (cube faces are square, so h is
	// ignored beyond storing it). rlgl can't allocate depth cubemaps, so this
	// uses raw GL (see the glad include note above).
	static raylib::RenderTexture2D LoadCubeTexture(int size, int /*h*/) {
		raylib::RenderTexture2D target(::RenderTexture2D(0));

		target.id = rlLoadFramebuffer();
		if (target.id == 0) {
			TRACELOG(LOG_WARNING, "GLRenderTarget: failed to create depth-cube framebuffer");
			return target;
		}

		// Create the depth cubemap: 6 faces of GL_DEPTH_COMPONENT (same alloc
		// convention as rlLoadTextureDepth: unsized format, driver picks bits).
		unsigned int cubeId = 0;
		glGenTextures(1, &cubeId);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeId);
		for (int face = 0; face < 6; ++face) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0,
				GL_DEPTH_COMPONENT, size, size, 0,
				GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
		}
		// NEAREST + clamp on all 3 axes (R too -- it's a cube). PCF is done in
		// the shader, so we sample raw per-texel depth.
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		// Attach + validate with RAW GL, all while the FBO is bound:
		//  - rlFramebufferAttach can't do it: its RL_ATTACHMENT_DEPTH case only
		//    handles TEXTURE2D/RENDERBUFFER; a cubemap face is silently ignored
		//    (and it unbinds the FBO on exit).
		//  - glDrawBuffer/glReadBuffer(GL_NONE) is stored PER-FBO, so it must be
		//    set while this FBO is bound. (rlActiveDrawBuffers(0) is a no-op.)
		// ShadowPass must re-attach each face the same way (raw GL) per render.
		glBindFramebuffer(GL_FRAMEBUFFER, target.id);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X, cubeId, 0); // +X first; pass rebinds per face
		glDrawBuffer(GL_NONE); // depth-only: no color to draw...
		glReadBuffer(GL_NONE); // ...or read
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			TRACELOG(LOG_WARNING, "GLRenderTarget: depth-cube framebuffer %u incomplete", target.id);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Cube id lives in the depth slot (DepthTextureId() hands it to the
		// shader). texture.id stays 0 -- width/height only feed the viewport
		// that BeginTextureMode sets.
		target.depth.id = cubeId;
		target.depth.width = size;
		target.depth.height = size;
		target.depth.format = 19; // DEPTH_COMPONENT
		target.depth.mipmaps = 1;
		target.texture.width = size;
		target.texture.height = size;
		return target;
	}

	static raylib::RenderTexture2D LoadDepthTexture(int w, int h)
	{
		raylib::RenderTexture2D target(::RenderTexture2D(0));

		target.id = rlLoadFramebuffer();
		if (target.id == 0)
		{
			TRACELOG(LOG_WARNING, "GLRenderTarget: failed to create depth framebuffer");
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
				"GLRenderTarget: depth framebuffer %u incomplete",
				target.id);
		}
		rlDisableFramebuffer();
		return target;
	}

	void GLRenderTarget::BindFace(int face) {
		if (m_format != Format::CUBE) return;
		rlDrawRenderBatchActive();
		glBindFramebuffer(GL_FRAMEBUFFER, m_texture.id);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, m_texture.depth.id, 0);
		rlViewport(0, 0, (int)width, (int)height);
		rlSetFramebufferWidth((int)width);
		rlSetFramebufferHeight((int)height);
	}

	void GLRenderTarget::EndCubeFace() {
		if (m_format != Format::CUBE) return;
		rlDrawRenderBatchActive();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());
		rlSetFramebufferWidth(GetScreenWidth());
		rlSetFramebufferHeight(GetScreenHeight());
	}

	void GLRenderTarget::SetFormat(Format fmt) {
		if (fmt == m_format) {
			return;
		}
		m_format = fmt;
		// Force the next Resize to reallocate in the new format.
		width = 0;
		height = 0;
	}

	void GLRenderTarget::Resize(uint32_t newWidth, uint32_t newHeight) {
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
		else if (m_format == Format::CUBE) {
			m_texture.Unload();
			m_texture = raylib::RenderTexture2D(LoadCubeTexture((int)width, (int)height));
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
		else if (m_format == Format::CUBE) {
			// Depth cubemap: filter/wrap (incl. WRAP_R) were set on the cube in
			// LoadCubeTexture. There's no 2D color texture to touch here.
		}
		else {
			SetTextureWrap(m_texture.texture, TEXTURE_WRAP_CLAMP);
			SetTextureFilter(m_texture.texture, TEXTURE_FILTER_BILINEAR);
		}
	}
} // namespace Long