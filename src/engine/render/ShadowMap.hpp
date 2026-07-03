#pragma once
#ifndef _SHADOW_MAP_HPP_
#define _SHADOW_MAP_HPP_
#include "raylib-cpp.hpp"
#include <cstdint>

namespace Long {
	// Depth-only render target for shadow mapping. raylib's RenderTexture attaches
	// depth as a RENDERBUFFER (not sampleable in shaders), so we build the FBO by
	// hand with rlgl: no color attachment, depth as a real TEXTURE the scene shader
	// can sample (u_shadowMap).
	class ShadowMap {
	public:
		ShadowMap() = default;
		~ShadowMap() { Unload(); }

		// Owns a GPU framebuffer -- no copies.
		ShadowMap(const ShadowMap&) = delete;
		ShadowMap& operator=(const ShadowMap&) = delete;

		// (Re)allocate at resolution x resolution. No-op if the size is unchanged.
		// Returns false if the framebuffer failed to complete.
		bool Resize(uint32_t resolution);

		// Draw depth into this map between Begin()/End(). Sets the FBO + viewport
		// and clears the depth buffer.
		void Begin();
		void End();

		unsigned int DepthTextureId() const { return m_depthTexId; }
		uint32_t Resolution() const { return m_resolution; }
		bool IsValid() const { return m_fboId != 0 && m_resolution > 0; }

	private:
		void Unload();
		unsigned int m_fboId{ 0 };      // framebuffer (depth attachment only)
		unsigned int m_depthTexId{ 0 }; // sampleable depth texture
		uint32_t m_resolution{ 0 };
	};
}
#endif // !_SHADOW_MAP_HPP_
