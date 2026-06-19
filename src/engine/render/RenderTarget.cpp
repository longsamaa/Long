#include "engine/render/RenderTarget.hpp"

namespace Long {
	void RenderTarget::Resize(uint32_t newWidth, uint32_t newHeight) {
		//If size not change
		if (newWidth == width && newHeight == height) {
			return;
		}
		if (newWidth == 0 || newHeight == 0) {
			return; // ignore degenerate sizes (e.g. minimized window)
		}

		width = newWidth;
		height = newHeight;
		// raylib::RenderTexture::Load unloads the previous texture first.
		m_texture.Load((int)width, (int)height);
	}
} // namespace Long