#include "ShadowMap.hpp"
#include "engine/Logger.hpp"
#include "rlgl.h"
#include <format>

namespace Long {

	bool ShadowMap::Resize(uint32_t resolution) {
		if (resolution == 0) {
			return false;
		}
		if (resolution == m_resolution && m_fboId != 0) {
			return true; // already allocated at this size
		}
		Unload();

		m_fboId = rlLoadFramebuffer(); // empty framebuffer
		if (m_fboId == 0) {
			Logger::TraceLog(::LOG_ERROR, "ShadowMap: failed to create framebuffer");
			return false;
		}
		rlEnableFramebuffer(m_fboId);
		m_depthTexId = rlLoadTextureDepth((int)resolution, (int)resolution, false);
		rlFramebufferAttach(m_fboId, m_depthTexId,
			RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
		if (!rlFramebufferComplete(m_fboId)) {
			Logger::TraceLog(::LOG_ERROR, "ShadowMap: framebuffer incomplete");
			rlDisableFramebuffer();
			Unload();
			return false;
		}
		rlDisableFramebuffer();
		rlTextureParameters(m_depthTexId, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_CLAMP);
		rlTextureParameters(m_depthTexId, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);
		m_resolution = resolution;
		return true;
	}

	void ShadowMap::Begin() {
		rlDrawRenderBatchActive();      // flush pending 2D/3D draws to the old target
		rlEnableFramebuffer(m_fboId);
		rlViewport(0, 0, (int)m_resolution, (int)m_resolution);
		rlSetFramebufferWidth((int)m_resolution);
		rlSetFramebufferHeight((int)m_resolution);
		// NOTE: the caller must have depth test + mask ON (ScopedDepthTest /
		// ScopedDepthMask) -- OpenGL does not write depth while GL_DEPTH_TEST is
		// off, and raylib leaves it off outside BeginMode3D.
		rlClearScreenBuffers();         // depth-only FBO: clears the depth buffer
	}

	void ShadowMap::End() {
		rlDrawRenderBatchActive();
		rlDisableFramebuffer();         // back to the previous target's FBO 0
		rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());
		rlSetFramebufferWidth(GetScreenWidth());
		rlSetFramebufferHeight(GetScreenHeight());
	}

	void ShadowMap::Unload() {
		if (m_depthTexId != 0) {
			rlUnloadTexture(m_depthTexId);
			m_depthTexId = 0;
		}
		if (m_fboId != 0) {
			rlUnloadFramebuffer(m_fboId);
			m_fboId = 0;
		}
		m_resolution = 0;
	}
}
