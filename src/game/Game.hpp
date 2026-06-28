#pragma once
#ifndef _GAME_HPP_
#define _GAME_HPP_
#include "engine/AppState.hpp"
#include "core/Scene.hpp"
#include "engine/camera/EditorCamera.hpp"
#include "engine/render/Renderer.hpp"
#include "engine/render/CommandQueue.hpp"
#include "engine/render/CommandDebugQueue.hpp"
#include "engine/Environment.hpp"
#include "engine/visibility/FrustumCulling.hpp"
#include "system/RenderStats.hpp"

namespace Long {
	class Application;
	class Game : public AppState {
	public:
		explicit Game(Application& app) : m_app(app) { m_state = State::GAME; }
		void OnEnter() override;
		void Update(float dt) override;
		void RenderWorld() override;
		Scene& GetScene() { return m_scene; }
		EditorCamera& GetCamera() { return m_camera; }
		void Execute(RenderContext& ctx) override;
		void copyhierarchy(const Scene& scene);
	private:
		void buildDefaultScene();
		Application& m_app;
		Scene m_scene;
		Environment m_environment;
		CommandQueue m_commandQueue;
		CommandDebugQueue m_commandDebugQueue;
		EditorCamera m_camera;
		Renderer m_renderer;
		FrustumCulling m_frustum;
		RenderStats m_renderStats;
	};
}
#endif // !_GAME_HPP_