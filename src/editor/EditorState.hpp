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
		void ReportEndDrawingMs(double ms) override { m_msEndDrawing = ms; }
	private:
		void testCreateDefaultCube();
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
		// Update-phase timings (ms). Measured in Update(), merged into m_renderStats
		// after RenderWorld() so Renderer's per-frame Reset() doesn't clobber them.
		double m_msTransformSystem = 0.0;
		double m_msPicking = 0.0;
		double m_msUpdate = 0.0;
		double m_msEndDrawing = 0.0; // previous frame's EndDrawing (from Application)
		RaycastHit m_hoverHit;                       // entity under the cursor this frame
		entt::entity m_selectedEntity = entt::null;  // entity clicked/selected (persists)
	};
}
#endif // !_EDITOR_STATE_HPP_