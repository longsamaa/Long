#pragma once
#ifndef _EDITOR_STATE_HPP_
#define _EDITOR_STATE_HPP_
#include "engine/AppState.hpp"
#include "engine/camera.hpp"
#include "core/Scene.hpp"
#include "editor/IPanel.hpp"
#include "system/RenderStats.hpp"
#include "system/RayCastSystem.hpp"
#include "engine/render/Renderer.hpp"
#include "engine/render/RenderContext.hpp"
#include "engine/render/CommandQueue.hpp"
#include "engine/render/CommandDebugQueue.hpp"
#include "engine/Environment.hpp"
#include <memory>
#include <vector>
namespace Long {
	class Application;
	class EditorState : public AppState {
	public:
		explicit EditorState(Application& app) : m_app(app) {}
		void OnEnter() override;
		void Update(float dt) override;
		void RenderWorld() override;
		void RenderUI() override;
	private:
		void testCreateDefaultCube();
		void RenderMenuBar();
		void RenderPanels();
		void UpdatePicking();
	private:
		Application& m_app;
		std::vector<std::unique_ptr<IPanel>> m_panels;
		Scene m_scene;
		Environment m_environment;
		CommandQueue m_commandQueue;
		CommandDebugQueue m_commandDebugQueue;
		EditorCamera m_camera;
		Renderer m_renderer;
		RenderStats m_renderStats;
		RaycastHit m_hoverHit;                       // entity under the cursor this frame
		entt::entity m_selectedEntity = entt::null;  // entity clicked/selected (persists)
	};
}
#endif // !_EDITOR_STATE_HPP_