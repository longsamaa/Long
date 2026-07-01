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
		Scene& m_scene;
		entt::entity& m_selected;
	};
}
#endif // !_HIERARCHY_PANEL_HPP_
