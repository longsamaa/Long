#pragma once
#ifndef _HIERARCHY_PANEL_HPP_
#define _HIERARCHY_PANEL_HPP_
#include "../IPanel.hpp"
#include <entt/entt.hpp>
namespace Long {
	class Scene;
	class HierarchyPanel : public IPanel {
	public:
		explicit HierarchyPanel(Scene& scene);
		~HierarchyPanel() = default;
		void render() override;
		void loadScene(Scene& scene); 
		void clear(); 
		entt::entity selected() const { return m_selected; }
	private:
		void drawEntityNode(entt::entity e);
		Scene& m_scene;
		entt::entity m_selected = entt::null;
	};
}
#endif // !_HIERARCHY_PANEL_HPP_
