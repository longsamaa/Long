#pragma once
#ifndef _INSPECTOR_PANEL_HPP_
#define _INSPECTOR_PANEL_HPP_
#include "editor/IPanel.hpp"
#include <entt/entt.hpp>
namespace Long {
	class Scene;
	class InspectorPanel : public IPanel {
	public:
		InspectorPanel(Scene& scene, const entt::entity& selected);
		~InspectorPanel() = default;
		void render() override;
	private:
		Scene& m_scene;
		const entt::entity& m_selected;
	};
}
#endif // !_INSPECTOR_PANEL_HPP_
