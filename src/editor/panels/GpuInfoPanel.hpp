#pragma once
#ifndef _GPU_INFO_PANEL_HPP_
#define _GPU_INFO_PANEL_HPP_
#include "../IPanel.hpp"
#include "engine/GpuInfo.hpp"
namespace Long {
	class GpuInfoPanel : public IPanel {
	public:
		GpuInfoPanel();
		~GpuInfoPanel() = default;

		void render() override;
	private:
		GpuInfo m_gpu;
	};
}
#endif // !_GPU_INFO_PANEL_HPP_
