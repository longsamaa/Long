#pragma once
#ifndef _GL_RENDER_TARGET_HPP_
#define _GL_RENDER_TARGET_HPP_
#include "RenderTarget.hpp"
#include "raylib-cpp.hpp"
namespace Long {
	// OpenGL implementation of RenderTarget: wraps a raylib RenderTexture (plus
	// hand-built FBOs for HDR / depth-texture / depth-cubemap formats) with RAII
	// and lazy resize. Owned by Renderer / passes; the rest of the engine only
	// sees the RenderTarget interface.
	class GLRenderTarget final : public RenderTarget {
	public:
		GLRenderTarget() = default;
		~GLRenderTarget() override = default;

		// No copies -- owns a GPU framebuffer.
		GLRenderTarget(const GLRenderTarget&) = delete;
		GLRenderTarget& operator=(const GLRenderTarget&) = delete;

		void SetFormat(Format fmt) override;
		void Resize(uint32_t width, uint32_t height) override;

		void Bind() override { m_texture.BeginMode(); }
		void Unbind() override { m_texture.EndMode(); }
		void BindFace(int face) override;
		void EndCubeFace() override;

		TextureHandle Color() const override {
			return { (uint64_t)m_texture.texture.id, width, height };
		}
		TextureHandle Depth() const override {
			return { (uint64_t)m_texture.depth.id, width, height };
		}

		// GL render textures are flipped vertically -> negative source height.
		raylib::Rectangle SourceRect() const override {
			return { 0.0f, 0.0f, (float)width, -(float)height };
		}

		uint32_t Width() const override { return width; }
		uint32_t Height() const override { return height; }
		bool IsValid() const override { return width > 0 && height > 0; }

	private:
		raylib::RenderTexture2D m_texture;
		uint32_t width{ 0 };
		uint32_t height{ 0 };
		Format m_format{ Format::LDR };
	};

	// Wrap a backend texture handle for the raylib-based passes (DrawTexturePro
	// & co). Purely a view -- nothing is owned. Only meaningful on the GL
	// backend, which is exactly where those passes run.
	inline raylib::TextureUnmanaged ToRaylibTexture(const TextureHandle& h) {
		::Texture2D t{};
		t.id = (unsigned int)h.id;
		t.width = (int)h.width;
		t.height = (int)h.height;
		t.mipmaps = 1;
		t.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
		return raylib::TextureUnmanaged(t);
	}
}
#endif // !_GL_RENDER_TARGET_HPP_