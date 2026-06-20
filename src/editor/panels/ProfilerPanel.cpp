#include "ProfilerPanel.hpp"
#include "imgui.h"
#include <raylib-cpp.hpp>

namespace Long {
	ProfilerPanel::ProfilerPanel(const RenderStats& stats)
		: m_stats(stats) {
		m_title = "Profiler";
		m_isOpen = true;
	}

	void ProfilerPanel::render() {
		if (!m_isOpen) {
			return;
		}
		if (ImGui::Begin(m_title.c_str(), &m_isOpen)) {
			ImGui::Text("FPS:        %d", ::GetFPS());
			ImGui::Text("Frame time: %.2f ms", ::GetFrameTime() * 1000.0f);
			ImGui::SeparatorText("Draw");
			ImGui::Text("Draw calls: %u", m_stats.drawCalls);
			ImGui::Text("Triangles:  %u", m_stats.triangles);
			ImGui::Text("Vertices:   %u", m_stats.vertices);
			ImGui::Text("Culled:     %u", m_stats.culledEntities);
			ImGui::Text("State switch:     %u", m_stats.stageCount);
			ImGui::Text("Num state:     %u", m_stats.materialCount);
			ImGui::Text("Render pass calls:     %u", m_stats.renderPassCalls);
		}
		ImGui::End();
	}
}