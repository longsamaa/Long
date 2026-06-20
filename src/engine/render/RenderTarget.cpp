#include "engine/render/RenderTarget.hpp"

namespace Long {
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
		m_texture.Load((int)width, (int)height);
	}
} // namespace Long