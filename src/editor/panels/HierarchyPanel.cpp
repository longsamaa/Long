#include "HierarchyPanel.hpp"
#include "core/Scene.hpp"
#include "core/Components.hpp"
#include "imgui.h"
#include <string>

namespace Long {
	HierarchyPanel::HierarchyPanel(Scene& scene, entt::entity& selected) : m_scene(scene), m_selected(selected) {
		m_title = "Scene hierarchy";
		m_isOpen = true;
	}
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
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}
		const bool open = ImGui::TreeNodeEx(
			reinterpret_cast<void*>(static_cast<intptr_t>(entt::to_integral(e))),
			flags, "%s", label.c_str());
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			m_selected = e;
		}
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			m_rClickItem = e;
		}

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Rename"))
			{
				m_RenameEntity = m_rClickItem;
				m_RenameFocusPending = true;
				Name& name = m_scene.Registry().get<Name>(m_RenameEntity);
				strncpy(m_RenameBuffer, name.value.c_str(), sizeof(m_RenameBuffer));
			}
			else if (ImGui::MenuItem("Delete")) {
				m_DestroyEntity = m_rClickItem;
			}
			ImGui::EndPopup();
		}

		if (m_RenameEntity != entt::null && m_RenameEntity == e) {
			ImGui::SetNextItemWidth(150);
			if (m_RenameFocusPending) {
				ImGui::SetKeyboardFocusHere();
				m_RenameFocusPending = false;
			}
			if (ImGui::InputText("##Rename", m_RenameBuffer,
				sizeof(m_RenameBuffer),
				ImGuiInputTextFlags_EnterReturnsTrue))
			{
				m_scene.Registry().patch<Name>(m_RenameEntity, [&](Name& name)
					{
						name.value = m_RenameBuffer;
					});
				m_RenameEntity = entt::null;
			}
			else if (ImGui::IsItemDeactivated()) {
				m_RenameEntity = entt::null;
			}
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
		if (m_DestroyEntity != entt::null) {
			if (m_rClickItem == m_DestroyEntity) {
				m_rClickItem = entt::null;
			}
			if (m_selected == m_DestroyEntity) {
				m_selected = entt::null;
			}
			if (m_rClickItem == m_DestroyEntity) {
				m_rClickItem = entt::null;
			}

			std::function<void(entt::entity)> destroyEntity;
			destroyEntity = [&](entt::entity e)
				{
					const Hierarchy* h = m_scene.Registry().try_get<Hierarchy>(e);
					if (!h || h->children.empty())
					{
						m_scene.Registry().destroy(e);
						return;
					}
					for (entt::entity child : h->children)
					{
						destroyEntity(child);
					}
					m_scene.Registry().destroy(e);
				};

			if (const Hierarchy* h = m_scene.Registry().try_get<Hierarchy>(m_DestroyEntity)) {
				if (!h->children.empty()) {
					for (entt::entity child : h->children) {
						m_scene.Registry().destroy(child);
					}
				}
			}

			m_scene.Registry().destroy(m_DestroyEntity);
			//Get parent
			m_DestroyEntity = entt::null;
		}
		if (ImGui::Begin(m_title.c_str(), &m_isOpen)) {
			std::string label_scene = m_scene.getSceneName();
			m_scene.getSceneName().empty() ? label_scene = "SampleScene" : label_scene = m_scene.getSceneName();
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
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

			if (ImGui::BeginPopupContextWindow("HierarchyContext",
				ImGuiPopupFlags_NoOpenOverItems))
			{
				if (ImGui::MenuItem("Create Empty"))
				{
					m_scene.CreateObject(CreateObjectType::GameObject);
				}

				if (ImGui::BeginMenu("3D Object"))
				{
					if (ImGui::MenuItem("Cube"))
					{
						m_scene.CreateObject(CreateObjectType::Cube); 
					}
					if (ImGui::MenuItem("Sphere"))
					{
						m_scene.CreateObject(CreateObjectType::Sphere); 
					}
					if (ImGui::MenuItem("Cylinder"))
					{
						m_scene.CreateObject(CreateObjectType::Cylinder); 
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
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
	void HierarchyPanel::appendCamera()
	{
	}
}