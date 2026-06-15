#include "GpuInfoPanel.hpp"
#include "imgui.h"
namespace Long {
	GpuInfoPanel::GpuInfoPanel() {
		this->m_title = "GPU Info";
		this->m_isOpen = false;
		m_gpu = GpuInfo::Query();
	}

	void GpuInfoPanel::render() {
		if (!m_isOpen) {
			return;
		}
		if (ImGui::Begin(m_title.c_str(), &m_isOpen)) {
			ImGui::Text("Renderer: %s", m_gpu.renderer.c_str());
			ImGui::Text("Vendor:   %s", m_gpu.vendor.c_str());
			ImGui::Text("GL:       %s", m_gpu.version.c_str());
		}
		ImGui::End();
	}
}
