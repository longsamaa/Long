#include "ProfilerPanel.hpp"
#include "imgui.h"
#include "raylib.h"

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
			ImGui::Text("FPS:        %d", GetFPS());
			ImGui::Text("Frame time: %.2f ms", GetFrameTime() * 1000.0f);

			ImGui::SeparatorText("Draw");
			ImGui::Text("Draw calls: %u", m_stats.drawCalls);
			ImGui::Text("Triangles:  %u", m_stats.triangles);
			ImGui::Text("Vertices:   %u", m_stats.vertices);
			ImGui::Text("Culled:     %u", m_stats.culledEntities);
		}
		ImGui::End();
	}
}
