#pragma once
#ifndef _HIERARCHY_PANEL_HPP_
#define _HIERARCHY_PANEL_HPP_
#include "editor/IPanel.hpp"
#include <entt/entt.hpp>
namespace Long {
	class Scene;
	class HierarchyPanel : public IPanel {
	public:
		explicit HierarchyPanel(Scene& scene, entt::entity& selected);
		~HierarchyPanel() = default;
		void render() override;
		void loadScene(Scene& scene);
		void clear();
		entt::entity selected() const { return m_selected; }
		void appendCamera();
	private:
		void drawEntityNode(entt::entity e);
		void reparentEntity(entt::entity child, entt::entity newParent);
		Scene& m_scene;
		entt::entity& m_selected;
		entt::entity m_rClickItem{ entt::null };
		entt::entity m_RenameEntity{ entt::null };
		entt::entity m_DestroyEntity{ entt::null };
		bool m_RenameFocusPending{ false }; // focus the input ONCE when rename starts
		char m_RenameBuffer[256] = {};
	};
}
#endif // !_HIERARCHY_PANEL_HPP_