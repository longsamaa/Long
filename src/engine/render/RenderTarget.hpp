#pragma once
#ifndef _RENDER_TARGET_HPP_
#define _RENDER_TARGET_HPP_
#include "raylib-cpp.hpp"
#include <cstdint>
namespace Long {
	// A render-to-texture surface for render passes. Wraps raylib::RenderTexture
	// with RAII + lazy resize: it (re)allocates the GPU texture only when the
	// requested size actually changes, so passes can call Resize() every frame
	// cheaply.
	class RenderTarget {
	public:
		RenderTarget() = default;
		~RenderTarget() = default;

		// No copies -- owns a GPU framebuffer.
		RenderTarget(const RenderTarget&) = delete;
		RenderTarget& operator=(const RenderTarget&) = delete;

		// Ensure the target is allocated at (width, height). No-op if unchanged.
		void Resize(uint32_t width, uint32_t height);

		// Draw into this target between Bind()/Unbind().
		void Bind() { m_texture.BeginMode(); }
		void Unbind() { m_texture.EndMode(); }

		// Color texture -- sample it in a post-process pass or blit to screen.
		raylib::TextureUnmanaged GetTexture() { return m_texture.GetTexture(); }

		uint32_t Width() const { return width; }
		uint32_t Height() const { return height; }
		bool IsValid() const { return width > 0 && height > 0; }

		// Source rect for drawing the texture: render textures are flipped
		// vertically in OpenGL, so height is negative.
		raylib::Rectangle SourceRect() const {
			return { 0.0f, 0.0f, (float)width, -(float)height };
		}

	private:
		raylib::RenderTexture m_texture;
		uint32_t width{ 0 };
		uint32_t height{ 0 };
	};
}
#endif // !_RENDER_TARGET_HPP_