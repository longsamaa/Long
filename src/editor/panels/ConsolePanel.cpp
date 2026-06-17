#include "ConsolePanel.hpp"
#include "engine/Logger.hpp"
#include "imgui.h"
#include "raylib.h"

namespace Long {

	ConsolePanel::ConsolePanel() {
		m_title = "Console";
		m_isOpen = true;
	}

	// Color per raylib log level.
	static ImVec4 LevelColor(int level) {
		switch (level) {
		case LOG_TRACE:   return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // gray
		case LOG_DEBUG:   return ImVec4(0.4f, 0.7f, 1.0f, 1.0f); // light blue
		case LOG_INFO:    return ImVec4(0.8f, 0.8f, 0.8f, 1.0f); // light gray
		case LOG_WARNING: return ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // yellow
		case LOG_ERROR:   return ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // red
		case LOG_FATAL:   return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // bright red
		default:          return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	void ConsolePanel::render() {
		if (!m_isOpen) {
			return;
		}
		if (ImGui::Begin(m_title.c_str(), &m_isOpen)) {
			// Toolbar.
			if (ImGui::Button("Clear")) {
				Logger::Clear();
			}
			ImGui::SameLine();
			ImGui::Checkbox("Auto-scroll", &m_autoScroll);
			ImGui::Separator();

			// Scrolling log region.
			if (ImGui::BeginChild("log", ImVec2(0, 0), ImGuiChildFlags_None,
								   ImGuiWindowFlags_HorizontalScrollbar)) {
				for (const LogEntry& e : Logger::Entries()) {
					ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(e.level));
					ImGui::TextUnformatted(e.text.c_str());
					ImGui::PopStyleColor();
				}
				// Stick to bottom when new logs arrive.
				if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
					ImGui::SetScrollHereY(1.0f);
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

}
