#pragma once
#ifndef _RENDER_TARGET_HPP_
#define _RENDER_TARGET_HPP_
#include "raylib-cpp.hpp"
#include <cstdint>
namespace Long {

	// Backend-opaque handle to a texture the backend owns. For the GL backend
	// `id` is the GL texture name; a Vulkan backend would stash its own handle
	// (e.g. an index into its image table). Passes hold/pass these around
	// without knowing what's inside.
	struct TextureHandle {
		uint64_t id{ 0 };
		uint32_t width{ 0 };
		uint32_t height{ 0 };
		bool IsValid() const { return id != 0; }
	};

	// Abstract render-to-texture surface. Passes only ever see this interface;
	// the concrete class (GLRenderTarget today, a VkRenderTarget later) owns the
	// actual framebuffer/attachments. Everything backend-specific -- FBO ids,
	// raylib RenderTexture, Y-flip conventions -- stays in the implementation.
	class RenderTarget {
	public:
		// HDR = half-float color (bloom/tonemap). DEPTH = depth-only, depth
		// sampleable (shadow maps). CUBE = depth cubemap (point-light shadows).
		enum class Format { LDR, HDR, DEPTH, CUBE };

		virtual ~RenderTarget() = default;

		// Choose the format the target (re)allocates with. Call before the first
		// Resize (or it forces a realloc on the next Resize).
		virtual void SetFormat(Format fmt) = 0;
		// Ensure the target is allocated at (width, height). No-op if unchanged.
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		// Draw into this target between Bind()/Unbind().
		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		// CUBE only: target face `face` (0..5, +X,-X,+Y,-Y,+Z,-Z), one render
		// per face; EndCubeFace() restores the previous framebuffer after the
		// last face.
		virtual void BindFace(int face) = 0;
		virtual void EndCubeFace() = 0;

		// Attachments for sampling in later passes. Depth() is only sampleable
		// for DEPTH/CUBE formats (2D targets keep depth in a renderbuffer).
		virtual TextureHandle Color() const = 0;
		virtual TextureHandle Depth() const = 0;

		// Source rect covering the whole color target, in the backend's sampling
		// convention (GL render textures are Y-flipped; Vulkan's wouldn't be).
		virtual raylib::Rectangle SourceRect() const = 0;

		virtual uint32_t Width() const = 0;
		virtual uint32_t Height() const = 0;
		virtual bool IsValid() const = 0;
	};
}
#endif // !_RENDER_TARGET_HPP_
