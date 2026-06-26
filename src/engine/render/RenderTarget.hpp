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

		// Choose the color format the target (re)allocates with. Call before the
		// first Resize (or it forces a realloc on the next Resize). HDR uses a
		// half-float RGBA buffer so values can exceed 1.0 -- required for bloom's
		// bright-pass / tonemapping. LDR is the normal 8-bit buffer.
		enum class Format { LDR, HDR };
		void SetFormat(Format fmt);

		// Ensure the target is allocated at (width, height). No-op if unchanged.
		void Resize(uint32_t width, uint32_t height);

		// Draw into this target between Bind()/Unbind().
		void Bind() { m_texture.BeginMode(); }
		void Unbind() { m_texture.EndMode(); }

		// Color texture (the image) -- sample it in a post-process pass or blit
		// to screen. Returned by value: a Texture is just ids/ints, cheap to copy.
		raylib::TextureUnmanaged GetTexture() { return m_texture.GetTexture(); }

		// The whole framebuffer (color + depth), e.g. if a pass needs the depth.
		raylib::RenderTexture2D& GetRenderTexture() { return m_texture; }

		uint32_t Width() const { return width; }
		uint32_t Height() const { return height; }
		bool IsValid() const { return width > 0 && height > 0; }

		// Source rect for drawing the texture: render textures are flipped
		// vertically in OpenGL, so height is negative.
		raylib::Rectangle SourceRect() const {
			return { 0.0f, 0.0f, (float)width, -(float)height };
		}

	private:
		raylib::RenderTexture2D m_texture;
		uint32_t width{ 0 };
		uint32_t height{ 0 };
		Format m_format{ Format::LDR };
	};
}
#endif // !_RENDER_TARGET_HPP_