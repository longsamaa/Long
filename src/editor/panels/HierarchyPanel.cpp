#include "HierarchyPanel.hpp"
#include "core/Scene.hpp"
#include "core/Components.hpp"
#include "imgui.h"
#include <string>

namespace Long {

	HierarchyPanel::HierarchyPanel(Scene& scene) : m_scene(scene) {
		m_title = "Hierarchy";
		m_isOpen = true;
	}

	// Recursively draw one entity and (if it has a Hierarchy) its children.
	void HierarchyPanel::drawEntityNode(entt::entity e) {
		const entt::registry& reg = m_scene.Registry();
		std::string label;
		if (const Name* name = reg.try_get<Name>(e)) {
			label = name->value;
		}
		if (label.empty()) {
			label = "Entity " + std::to_string(entt::to_integral(e));
		}
		const Hierarchy* hier = reg.try_get<Hierarchy>(e);
		const bool hasChildren = hier && !hier->children.empty();
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
								   ImGuiTreeNodeFlags_SpanAvailWidth;
		if (m_selected == e) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}
		if (!hasChildren) {
			// Leaf: no arrow, can't expand.
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}
		const bool open = ImGui::TreeNodeEx(
			reinterpret_cast<void*>(static_cast<intptr_t>(entt::to_integral(e))),
			flags, "%s", label.c_str());
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			m_selected = e;
		}
		if (open && hasChildren) {
			for (entt::entity child : hier->children) {
				drawEntityNode(child);
			}
			ImGui::TreePop();
		}
	}

	void HierarchyPanel::render() {
		if (!m_isOpen) {
			return;
		}
		if (ImGui::Begin(m_title.c_str(), &m_isOpen)) {
			std::string label_scene = m_scene.getSceneName();
			m_scene.getSceneName().empty() ? label_scene = "SampleScene" : label_scene = m_scene.getSceneName();
			ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
			const bool sceneOpen = ImGui::TreeNodeEx(
				static_cast<void*>(&m_scene),
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_SpanAvailWidth, "%s", label_scene.c_str());
			if (sceneOpen) {
				const entt::registry& reg = m_scene.Registry();
				reg.view<entt::entity>().each([&](entt::entity e) {
					const Hierarchy* hier = reg.try_get<Hierarchy>(e);
					if (!hier || hier->parent == entt::null) {
						drawEntityNode(e);
					}
				});
				ImGui::TreePop();
			}
			// Click in empty space clears the selection.
			if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
				!ImGui::IsAnyItemHovered()) {
				m_selected = entt::null;
			}
		}
		ImGui::End();
	}

	void HierarchyPanel::loadScene(Scene& scene)
	{

	}

	void HierarchyPanel::clear()
	{
	}

}
