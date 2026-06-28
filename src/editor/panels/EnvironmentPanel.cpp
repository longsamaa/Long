#include "EnvironmentPanel.hpp"
#include "engine/Environment.hpp"
#include "imgui.h"

namespace Long {
	EnvironmentPanel::EnvironmentPanel(Environment& env)
		: m_env(env) {
		m_title = "Environment";
		m_isOpen = true;
	}

	void EnvironmentPanel::render() {
		if (!m_isOpen) {
			return;
		}
		if (ImGui::Begin(m_title.c_str(), &m_isOpen)) {
			ImGui::SeparatorText("Skybox Gradient");
			ImGui::ColorEdit3("Top color", &m_env.topColor.x);
			ImGui::ColorEdit3("Bottom color", &m_env.bottomColor.x);
			ImGui::SliderFloat("Horizon sharpness", &m_env.gradientSharpness, 0.0f, 1.0f);
		}
		ImGui::End();
	}
}