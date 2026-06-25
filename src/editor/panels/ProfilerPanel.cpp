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
			ImGui::SeparatorText("ScenePass timing (ms)");
			ImGui::Text("RenderSystem: %.3f", m_stats.msRenderSystem);
			ImGui::Text("Sort:         %.3f", m_stats.msSort);
			ImGui::Text("BuildBatches: %.3f", m_stats.msBuildBatches);
			ImGui::Text("Execute:      %.3f", m_stats.msExecute);
			ImGui::Text("ScenePass:    %.3f", m_stats.msScenePass);
			ImGui::SeparatorText("Update timing (ms)");
			ImGui::Text("TransformSys: %.3f", m_stats.msTransformSystem);
			ImGui::Text("Picking:      %.3f", m_stats.msPicking);
			ImGui::Text("Update total: %.3f", m_stats.msUpdate);
			ImGui::SeparatorText("Pass timing (ms)");
			static const char* kPassNames[] = {
				"Scene", "Mask", "Composite", "Outline", "FXAA", "Gizmo"
			};
			for (int i = 0; i < m_stats.passCount; ++i) {
				const char* name = (i < (int)(sizeof(kPassNames) / sizeof(*kPassNames)))
					? kPassNames[i] : "Pass";
				ImGui::Text("%-12s %.3f", name, m_stats.msPass[i]);
			}
			ImGui::Text("Render total: %.3f", m_stats.msRenderTotal);
			ImGui::Text("EndDrawing:   %.3f", m_stats.msEndDrawing);
		}
		ImGui::End();
	}
}