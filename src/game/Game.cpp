#include "game/Game.hpp"
#include "engine/Application.hpp"
#include "core/Components.hpp"
#include "system/TransformSystem.hpp"
#include "system/WorldBoundSystem.hpp"
#include "system/GameCameraSystem.hpp"
#include "engine/render/RenderContext.hpp"
#include "engine/render/passes/ScenePreparePass.hpp"
#include "engine/render/passes/ShadowPass.hpp"
#include "engine/render/passes/ScenePass.hpp"
#include "engine/render/passes/BrightPass.hpp"
#include "engine/render/passes/BloomPass.hpp"
#include "engine/render/passes/BloomCompositePass.hpp"
#include "engine/render/passes/TonemapPass.hpp"
#include "raylib-cpp.hpp"

namespace Long {
	void Game::setCamera(GameCamera camera)
	{
		m_camera = camera; 
	}
	void Game::OnEnter() {
		m_scene.SetApplication(&m_app);
		m_renderer.AddPass(std::make_unique<ScenePreparePass>());
		m_renderer.AddPass(std::make_unique<ShadowPass>());
		m_renderer.AddPass(std::make_unique<ScenePass>());
		m_renderer.AddPass(std::make_unique<BrightPass>());
		m_renderer.AddPass(std::make_unique<BloomPass>());
		m_renderer.AddPass(std::make_unique<BloomCompositePass>());
		m_renderer.AddPass(std::make_unique<TonemapPass>());
		m_environment.Init(m_app.GetAssets());
		m_frustum.setCamera(&m_camera);
		//buildDefaultScene();
	}

	void Game::Update(float dt) {
		m_camera.Update(dt);
		TransformSystem(m_scene.Registry());
		WorldBoundsSystem(m_scene.Registry(), m_app.GetAssets());
		GameCameraSystem(m_scene.Registry(), m_camera);
		//Update m_light nap lại vào context render
		LightSystem(m_scene.Registry(), m_lights);
	}

	void Game::BeginFrame(){
		m_commandDebugQueue.Clear(); 
		m_commandQueue.Clear(); 
	}

	void Game::RenderWorld() {
		RenderContext ctx;
		ctx.commandQueue = &m_commandQueue;
		ctx.commandDebugQueue = &m_commandDebugQueue;
		ctx.environment = &m_environment;
		ctx.registry = &m_scene.Registry();
		ctx.assets = &m_app.GetAssets();
		ctx.camera = &m_camera;
		ctx.frustum = &m_frustum;
		ctx.width = (uint32_t)GetRenderWidth();
		ctx.height = (uint32_t)GetRenderHeight();
		Execute(ctx);
		m_renderStats = ctx.renderStats;
	}

	void Game::RenderUI()
	{}

	void Game::EndFrame(){
		//clear command 
		m_commandQueue.Clear(); 
		m_commandDebugQueue.Clear(); 
	}

	void Game::OnExit()
	{}

	void Game::Execute(RenderContext& ctx) {
		m_renderer.Render(ctx);
	}

	void Game::copyhierarchy(const Scene& scene)
	{
		m_scene.Clone(scene);
	}

	void Game::buildDefaultScene() {
		auto& assets = m_app.GetAssets();
		auto& reg = m_scene.Registry();
		raylib::Mesh cube = raylib::Mesh::Cube(1.0f, 1.0f, 1.0f);
		raylib::BoundingBox box(cube);
		uint32_t meshId = assets.AddMesh(std::move(cube));
		uint32_t emissiveId = assets.GetShaderId("emissive");
		uint32_t mat = assets.CreateEmissiveMaterial(emissiveId, raylib::Color{ 80, 180, 255, 255 }, 5.0f);
		entt::entity e = m_scene.CreateEntity("cube");
		Transform t;
		t.position = { 0.0f, 1.0f, 0.0f };
		reg.emplace<Transform>(e, t);
		reg.emplace<MeshFilter>(e, MeshFilter{ meshId });
		reg.emplace<MeshRenderer>(e, MeshRenderer{ mat, raylib::Color::White(), true });
		reg.emplace<BoxCollider3D>(e, BoxCollider3D{ box });
		// on_construct<Transform> already marked it dirty.
	}
}