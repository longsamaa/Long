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
#include "engine/EditorGizmo.hpp"
#include "engine/visibility/FrustumCulling.hpp"
#include "game/game.hpp"
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
		void createGround();
		void createEmissiveBoxes(); // 4 glowing boxes spread out, to test bloom/HDR
		void RenderMenuBar();
		void RenderPanels();
		void RenderGizmoToolbar();
		void UpdatePicking();
	private:
		Application& m_app;
		std::vector<std::unique_ptr<IPanel>> m_panels;
		Scene m_scene;
		Environment m_environment;
		CommandQueue m_commandQueue;
		CommandDebugQueue m_commandDebugQueue;
		EditorCamera m_camera;
		EditorGizmo m_gizmo;
		Renderer m_renderer;
		RenderStats m_renderStats;
		//visibility
		FrustumCulling m_frustum; 
		double m_msTransformSystem = 0.0;
		double m_msUpdate = 0.0;
		RaycastHit m_hoverHit;                       // entity under the cursor this frame
		entt::entity m_selectedEntity = entt::null;  // entity clicked/selected (persists)
		std::unique_ptr<Game> m_gameState{ nullptr }; //game state 
	};
}
#endif // !_EDITOR_STATE_HPP_