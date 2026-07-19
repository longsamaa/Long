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

			ImGui::SeparatorText("Bloom");
			ImGui::SliderFloat("Threshold", &m_env.bloomThreshold, 0.5f, 5.0f, "%.2f");
			ImGui::SliderFloat("Soft knee", &m_env.bloomSoftKnee, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat("Clamp max", &m_env.bloomClampMax, 1.0f, 20.0f, "%.1f");
			ImGui::SliderFloat("Strength", &m_env.bloomStrength, 0.0f, 2.0f, "%.2f");
		}
		ImGui::End();
	}
}