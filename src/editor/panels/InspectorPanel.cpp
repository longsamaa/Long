#include "InspectorPanel.hpp"
#include "editor/inspector/CameraParameterInspector.hpp"
#include "editor/inspector/TransformInspector.hpp"
#include "core/Scene.hpp"
#include "core/Components.hpp"
#include "imgui.h"

namespace Long {

	InspectorPanel::InspectorPanel(Scene& scene, const entt::entity& selected)
		: m_scene(scene), m_selected(selected) {
		m_title = "Inspector";
		m_isOpen = true;
	}

	void InspectorPanel::render() {
		if (!m_isOpen) {
			return;
		}
		if (ImGui::Begin(m_title.c_str(), &m_isOpen)) {
			entt::registry& reg = m_scene.Registry();

			if (m_selected == entt::null || !reg.valid(m_selected)) {
				ImGui::TextDisabled("No entity selected.");
				ImGui::End();
				return;
			}
			if (const Name* name = reg.try_get<Name>(m_selected)) {
				ImGui::TextUnformatted(name->value.c_str());
			} else {
				ImGui::Text("Entity %u", entt::to_integral(m_selected));
			}
			ImGui::Separator();
			if (GameCameraParameter* cam = reg.try_get<GameCameraParameter>(m_selected)) {
				if (ImGui::CollapsingHeader("Game Camera Parameter",
											ImGuiTreeNodeFlags_DefaultOpen)) {
					CameraParameterInspector::Draw(*cam);
				}
			}
			if (Transform* transform = reg.try_get<Transform>(m_selected)) {
				if (ImGui::CollapsingHeader("Transform",
					ImGuiTreeNodeFlags_DefaultOpen)) {
					TransformInspector::Draw(*transform);
				}
			}
		}
		ImGui::End();
	}

}
