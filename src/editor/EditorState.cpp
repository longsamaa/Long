#include "editor/EditorState.hpp"
#include "engine/Application.hpp"
#include "editor/panels/GpuInfoPanel.hpp"
#include "editor/panels/ProfilerPanel.hpp"
#include "editor/panels/ConsolePanel.hpp"
#include "editor/panels/HierarchyPanel.hpp"
#include "editor/panels/InspectorPanel.hpp"
#include "editor/panels/EnvironmentPanel.hpp"
#include "core/Components.hpp"
#include "system/RenderSystem.hpp"
#include "system/TransformSystem.hpp"
#include "system/RayCastSystem.hpp"
#include "system/WorldBoundSystem.hpp"
#include "system/GameCameraSystem.hpp"
#include "system/LightSystem.hpp"
#include "engine/render/passes/ScenePreparePass.hpp"
#include "engine/render/passes/ShadowPass.hpp"
#include "engine/render/passes/ShadowDebugPass.hpp"
#include "engine/render/passes/ScenePass.hpp"
#include "engine/render/passes/MaskPass.hpp"
#include "engine/render/passes/CompositePass.hpp"
#include "engine/render/passes/OutlinePass.hpp"
#include "engine/render/passes/FXAAPass.hpp"
#include "engine/render/passes/GizmoPass.hpp"
#include "engine/render/passes/BrightPass.hpp"
#include "engine/render/passes/BloomPass.hpp"
#include "engine/render/passes/BloomCompositePass.hpp"
#include "engine/render/passes/TonemapPass.hpp"
#include "engine/render/passes/RenderDebugPass.hpp"
#include "helpers/draw_debug_helper.hpp"
#include "core/math/transform.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "raylib-cpp.hpp"
#include "helpers/TimerHelper.hpp"
#include "engine/Logger.hpp"
#include <iostream>
namespace Long {
	void EditorState::OnEnter() {
		m_scene.SetApplication(&m_app);
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: begin, creating panels");
		m_panels.push_back(std::make_unique<GpuInfoPanel>());
		m_panels.push_back(std::make_unique<ProfilerPanel>(m_renderStats));
		m_panels.push_back(std::make_unique<ConsolePanel>());
		m_panels.push_back(std::make_unique<HierarchyPanel>(m_scene, m_selectedEntity));
		m_panels.push_back(std::make_unique<InspectorPanel>(m_scene, m_selectedEntity));
		for (auto& panel : m_panels) {
			if (!(panel->title() == "Scene hierarchy")
				&& !(panel->title() == "Inspector")) {
				panel->close();
			}
		}

		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: panels done, adding render passes");
		m_renderer.AddPass(std::make_unique<ScenePreparePass>());   // gather + sort + build batches (once)
		m_renderer.AddPass(std::make_unique<ShadowPass>());         // depth-from-light -> shadow map
		m_renderer.AddPass(std::make_unique<ScenePass>());          // scene -> sceneTarget (HDR)
		m_renderer.AddPass(std::make_unique<MaskPass>());           // selection mask
		m_renderer.AddPass(std::make_unique<BrightPass>());         // sceneTarget -> brightTarget
		m_renderer.AddPass(std::make_unique<BloomPass>());          // brightTarget -> blurTarget (mip-chain bloom)
		m_renderer.AddPass(std::make_unique<BloomCompositePass>()); // scene+bloom -> finalTarget (HDR)
		m_renderer.AddPass(std::make_unique<TonemapPass>());        // HDR -> screen (tonemap + FXAA)
		m_renderer.AddPass(std::make_unique<OutlinePass>());        // overlay, straight to screen
		m_renderer.AddPass(std::make_unique<GizmoPass>());          // overlay, straight to screen
		m_renderer.AddPass(std::make_unique<RenderDebugPass>());          // overlay, render debug
		//m_renderer.AddPass(std::make_unique<ShadowDebugPass>());          // DEBUG: blit shadow map to corner

		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: passes added, init environment");
		m_environment.Init(m_app.GetAssets());
		m_panels.push_back(std::make_unique<EnvironmentPanel>(m_environment));
		m_panels.back()->close();
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter : create main camera");
		createComponentCamera();
		createComponentLight();
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: createGround begin");
		createGround();
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: createGround done, createEmissiveBoxes begin");
		createEmissiveBoxes();
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: createEmissiveBoxes done");
		createRobot(); 
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: createRobot done");
	}

	void EditorState::createComponentCamera()
	{
		auto& reg = m_scene.Registry();
		auto& assets = m_app.GetAssets();
		entt::entity main_camera = m_scene.CreateEntity("MainCamera");
		const raylib::Vector3& camera_pos = m_gameCamera.Raw().GetPosition();
		Transform transform;
		reg.emplace<Transform>(main_camera, transform);
		raylib::Mesh sphere = raylib::Mesh::Sphere(0.5f, 16, 16);
		raylib::BoundingBox box_collider(sphere);
		uint32_t meshId = assets.AddMesh(std::move(sphere));
		uint32_t defaultId = assets.GetShaderId("default");
		uint32_t mat = assets.CreateDefaultMaterial(defaultId, raylib::Color{ 200, 200, 200, 255 });
		// Editor helper mesh: must not occlude light or the gizmo sphere throws
		// stray shadows into the scene.
		assets.GetMaterial(mat).SetCastShadow(false);
		reg.emplace<MeshFilter>(main_camera, MeshFilter{ meshId });
		reg.emplace<MeshRenderer>(main_camera, MeshRenderer{ mat, raylib::Color::White(), true });
		reg.emplace<BoxCollider3D>(main_camera, BoxCollider3D{ box_collider });
		reg.emplace<Hierarchy>(main_camera, Hierarchy{ entt::null, {} });
		reg.emplace<GameCameraParameter>(main_camera, GameCameraParameter{});
		reg.emplace<MainCamera>(main_camera); // empty tag: emplace takes no value
	}

	void EditorState::createComponentLight()
	{
		auto& reg = m_scene.Registry();
		auto& assets = m_app.GetAssets();
		entt::entity light = m_scene.CreateEntity("DirectionalLight");
		Transform transform;
		transform.position = { 10.0f, 10.0f, 0.0f };
		reg.emplace<Transform>(light, transform);
		raylib::Mesh sphere = raylib::Mesh::Sphere(0.5f, 16, 16);
		raylib::BoundingBox box_collider(sphere);
		uint32_t meshId = m_app.GetAssets().AddMesh(std::move(sphere));
		uint32_t defaultId = assets.GetShaderId("default");
		uint32_t mat = assets.CreateDefaultMaterial(defaultId, raylib::Color::Yellow());
		reg.emplace<MeshFilter>(light, MeshFilter{ meshId });
		reg.emplace<MeshRenderer>(light, MeshRenderer{ mat, raylib::Color::White(), true });
		reg.emplace<Hierarchy>(light, Hierarchy{ entt::null, {} });
		reg.emplace<LightComponent>(light, LightComponent{
			.type = LightType::Directional,
			.direction = { -1.0f, -1.0f, 0.0f },
			.color = { 255, 255, 255, 255 },
			.intensity = 1.0f,
			.castsShadows = true,
			});
	}

	void EditorState::Update(float dt) {
		!m_game ? EditorModeUpdate(dt) : GameModeUpdate(dt);
	}

	void EditorState::UpdatePicking() {
		if (ImGui::GetIO().WantCaptureMouse) {
			m_hoverHit = RaycastHit{};
			return;
		}
		if (raylib::Mouse::IsButtonDown(MOUSE_BUTTON_LEFT)) {
			raylib::Ray ray = ::GetScreenToWorldRay(GetMousePosition(), m_camera.Raw());
			auto tPick = Time::now();
			m_hoverHit = RaycastSystem(m_scene.Registry(), ray);
			if (m_hoverHit.hit) {
				m_selectedEntity = m_hoverHit.entity;
			}
			else {
				m_selectedEntity = entt::null;
			}
		}
		float wheel = ::GetMouseWheelMove();
		if (wheel != 0.0f) {
			raylib::Ray ray = ::GetScreenToWorldRay(GetMousePosition(), m_camera.Raw());
			raylib::Vector3 pivot;
			if (m_hoverHit.hit) {
				pivot = m_hoverHit.point;
			}
			else if (fabsf(ray.direction.y) > 1e-5f) {
				float t = -ray.position.y / ray.direction.y;
				pivot = raylib::Vector3(ray.position).Add(raylib::Vector3(ray.direction).Scale(t > 0 ? t : 0));
			}
			else {
				pivot = m_camera.Raw().target;
			}
			m_camera.ZoomToward(wheel, pivot);
		}
	}

	void EditorState::EditorModeUpdate(const float& dt)
	{
		if (!ImGui::GetIO().WantCaptureMouse) {
			m_camera.Update(dt);
		}
		m_commandQueue.Clear();

		auto t0 = Time::now();
		TransformSystem(m_scene.Registry());
		WorldBoundsSystem(m_scene.Registry(), m_app.GetAssets());
		GameCameraSystem(m_scene.Registry(), m_gameCamera);
		LightSystem(m_scene.Registry(), m_lights);
		m_msTransformSystem = Time::elapsedSecond(t0);

		bool gizmoHasInput = false;
		if (m_selectedEntity != entt::null && m_scene.Registry().valid(m_selectedEntity)
			&& m_scene.Registry().all_of<Transform>(m_selectedEntity)
			&& !ImGui::GetIO().WantCaptureMouse) {
			m_gizmo.Update(m_camera, m_scene, m_selectedEntity);
			gizmoHasInput = m_gizmo.IsActive() || m_gizmo.IsHot();
		}
		if (!gizmoHasInput) {
			UpdatePicking();
		}
	}

	void EditorState::GameModeUpdate(float dt)
	{
		m_game->Update(dt);
	}

	void EditorState::EditorModeRenderWorld()
	{
		//Build Context
		RenderContext ctx;
		ctx.commandQueue = &m_commandQueue;
		ctx.commandDebugQueue = &m_commandDebugQueue;
		ctx.environment = &m_environment;
		ctx.registry = &m_scene.Registry();
		ctx.assets = &m_app.GetAssets();
		ctx.camera = &m_camera;
		ctx.frustum = &m_frustum;
		ctx.frustum->setCamera(&m_camera);
		ctx.lights = &m_lights;
		ctx.width = (uint32_t)::GetRenderWidth();
		ctx.height = (uint32_t)::GetRenderHeight();
		AddDebug(ctx);
		Execute(ctx);
		m_renderStats = ctx.renderStats;
		m_renderStats.msTransformSystem = m_msTransformSystem;
		m_renderStats.msUpdate = m_msUpdate;
	}

	void EditorState::GameModeRenderWorld()
	{
		m_game->RenderWorld();
	}

	void EditorState::AddDebug(RenderContext& ctx)
	{
		if (m_selectedEntity != entt::null && m_scene.Registry().valid(m_selectedEntity)) {
			ctx.selectedEntities = { m_selectedEntity };
			auto& reg = m_scene.Registry();
			if (reg.all_of<Transform, MatrixTransform>(m_selectedEntity)) {
				ctx.gizmo = &m_gizmo;
				m_gizmoWorldT = DecomposeToTransform(
					reg.get<MatrixTransform>(m_selectedEntity).world_matrix);
				ctx.gizmoTarget = &m_gizmoWorldT;
			}
		}

		if (m_scene.Registry().try_get<MainCamera>(m_selectedEntity)) {
			auto& reg = m_scene.Registry();
			const Transform& t = reg.get<Transform>(m_selectedEntity);
			const raylib::Camera3D& cam = m_gameCamera.Raw();
			CameraHelperParams params{
				cam.GetPosition(), cam.GetTarget(), cam.GetUp(),
				cam.GetFovy(), m_gameCamera.Near(), m_gameCamera.Far(),
				(int)cam.projection
			};
			m_commandDebugQueue.Submit(BuildCameraHelperCommand(params));
		}

		if (const LightComponent* lc = m_scene.Registry().try_get<LightComponent>(m_selectedEntity)) {
			const Transform& t = m_scene.Registry().get<Transform>(m_selectedEntity);
			m_commandDebugQueue.Submit(LightHelperCommand{
				t.position, lc->world_direction, lc->color, 4.0f });
		}
	}

	IPanel* EditorState::getPanel(const std::string& name)
	{
		auto it = std::find_if(m_panels.begin(), m_panels.end(), [name](const std::unique_ptr<IPanel>& panel) {
			return  (panel->title() == name);
			});
		return (it != m_panels.end() ? it->get() : nullptr);
	}

	void EditorState::RenderWorld() {
		!m_game ? EditorModeRenderWorld() : GameModeRenderWorld();
	}

	void EditorState::BeginFrame() {
		m_commandQueue.Clear();
		m_commandDebugQueue.Clear();
	}

	void EditorState::EndFrame() {
	}

	void EditorState::Execute(RenderContext& ctx)
	{
		m_renderer.Render(ctx);
	}

	void EditorState::createGround()
	{
		auto& assets = m_app.GetAssets();
		auto& reg = m_scene.Registry();
		raylib::Mesh cube = raylib::Mesh::Cube(4.0f, 4.0f, 4.0f);
		raylib::BoundingBox box(cube);
		uint32_t meshId = assets.AddMesh(std::move(cube));
		uint32_t wireId = assets.GetShaderId("wireframe");
		raylib::Color lightBrown{ 196, 164, 132, 255 };
		raylib::Color faceBrown{ 120, 96, 72, 255 };
		uint32_t mat = assets.CreateWireframeMaterial(wireId, lightBrown, faceBrown, 0.02f);
		// NOTE: cast shadow stays ON. The material is shared by every tile --
		// including ones the user lifts up -- and a non-casting mesh lets light
		// pass straight through it ("bong xuyen vat the"). Self-shadow acne on
		// the flat ground is handled by the slope-scaled bias in the shader.
		entt::entity parent = m_scene.CreateEntity("ground");
		reg.emplace<Transform>(parent, Transform{});
		std::vector<entt::entity> children;
		const int N = 50;
		const float spacing = 4.0f;
		for (int x = 0; x < N; ++x) {
			for (int z = 0; z < N; ++z) {
				entt::entity tile = m_scene.CreateEntity("tile");
				Transform t;
				t.position = { (x - N / 2) * spacing, -2.0f, (z - N / 2) * spacing };
				reg.emplace<Transform>(tile, t);
				reg.emplace<Hierarchy>(tile, Hierarchy{ parent, {} });
				reg.emplace<MeshFilter>(tile, MeshFilter{ meshId });
				reg.emplace<MeshRenderer>(tile, MeshRenderer{ mat, raylib::Color::White(), true });
				reg.emplace<BoxCollider3D>(tile, BoxCollider3D{ box });
				children.push_back(tile);
			}
		}
		reg.emplace<Hierarchy>(parent, Hierarchy{ entt::null, std::move(children) });
	}

	void EditorState::createEmissiveBoxes()
	{
		auto& assets = m_app.GetAssets();
		auto& reg = m_scene.Registry();
		raylib::Mesh cube = raylib::Mesh::Cube(1.0f, 1.0f, 1.0f);
		raylib::BoundingBox box(cube);
		uint32_t meshId = assets.AddMesh(std::move(cube));
		uint32_t emissiveId = assets.GetShaderId("emissive");
		struct Glow { raylib::Vector3 pos; raylib::Color color; };
		const Glow glows[4] = {
			{ {  8.0f, 1.0f,  8.0f }, raylib::Color{ 255,  40,  40, 255 } }, // red
			{ { -8.0f, 1.0f,  8.0f }, raylib::Color{  40, 255,  40, 255 } }, // green
			{ {  8.0f, 1.0f, -8.0f }, raylib::Color{  40, 120, 255, 255 } }, // blue
			{ { -8.0f, 1.0f, -8.0f }, raylib::Color{ 255, 220,  40, 255 } }, // yellow
		};
		for (const Glow& g : glows) {
			uint32_t emat = assets.CreateEmissiveMaterial(emissiveId, g.color, 5.0f);
			entt::entity e = m_scene.CreateEntity("emissive");
			Transform t;
			t.position = g.pos;
			t.scale = { 0.5f, 0.5f, 0.5f }; // bigger so the glow reads
			reg.emplace<Transform>(e, t);
			reg.emplace<MeshFilter>(e, MeshFilter{ meshId });
			reg.emplace<MeshRenderer>(e, MeshRenderer{ emat, raylib::Color::White(), true });
			reg.emplace<BoxCollider3D>(e, BoxCollider3D{ box });
			reg.emplace<Hierarchy>(e, Hierarchy{ entt::null, {} });
			// on_construct<Transform> already marked it dirty.
		}
	}

	void EditorState::createRobot()
	{
		auto& assets = m_app.GetAssets(); 
		auto& reg = m_scene.Registry(); 
		std::filesystem::path robotDir =
			std::filesystem::path(GetApplicationDirectory()) / "resources/robot.glb";
		auto robot = m_app.GetAssets().ImportModel(robotDir); 
		uint32_t defaultId = assets.GetShaderId("default");
		uint32_t emat = assets.CreateDefaultMaterial(defaultId, raylib::Color{ 192, 192, 192, 255 });
		entt::entity parent = m_scene.CreateEntity("robot");
		reg.emplace<Transform>(parent, Transform{});
		auto& parent_hierarchy = reg.emplace<Hierarchy>(parent, Hierarchy{ entt::null, {} });
		for (const auto& meshId : robot.meshIds) {
			entt::entity e = m_scene.CreateEntity(robot.meshName[meshId]);
			auto& mesh = m_app.GetAssets().GetMesh(meshId); 
			reg.emplace<Transform>(e, Transform{});
			reg.emplace<MeshFilter>(e, MeshFilter{ meshId });
			reg.emplace<MeshRenderer>(e, MeshRenderer{ emat, raylib::Color::White(), true });
			reg.emplace<BoxCollider3D>(e, BoxCollider3D{ raylib::BoundingBox(mesh)});
			reg.emplace<Hierarchy>(e, Hierarchy{ parent, {} });
			parent_hierarchy.children.push_back(e); 
		}
	}

	void EditorState::RenderMenuBar()
	{
		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Quit")) {
					m_app.Quit();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window")) {
				for (auto& panel : m_panels) {
					ImGui::MenuItem(panel->title().c_str(), nullptr, &panel->isOpen());
				}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	}

	void EditorState::RenderPanels()
	{
		for (auto& panel : m_panels) {
			panel->render();
		}
	}

	void EditorState::RenderGizmoToolbar() {
		if (!ImGui::GetIO().WantCaptureKeyboard) {
			if (ImGui::IsKeyPressed(ImGuiKey_W)) m_gizmo.SetMode(EditorGizmo::Mode::Translate);
			if (ImGui::IsKeyPressed(ImGuiKey_E)) m_gizmo.SetMode(EditorGizmo::Mode::Rotate);
			if (ImGui::IsKeyPressed(ImGuiKey_R)) m_gizmo.SetMode(EditorGizmo::Mode::Scale);
		}
		const float pad = 8.0f;
		const ImGuiViewport* vp = ImGui::GetMainViewport();
		ImVec2 origin = vp->WorkPos; // fallback: viewport work area
		if (ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(m_app.GetDockspaceId())) {
			origin = central->Pos;
		}
		ImVec2 pos{ origin.x + pad, origin.y + pad };
		ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
		ImGui::SetNextWindowViewport(vp->ID);
		ImGui::SetNextWindowBgAlpha(0.65f);
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav
			| ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDocking;

		if (ImGui::Begin("##GizmoToolbar", nullptr, flags)) {
			struct ModeBtn { const char* label; EditorGizmo::Mode mode; };
			const ModeBtn buttons[] = {
				{ "Move",   EditorGizmo::Mode::Translate },
				{ "Rotate", EditorGizmo::Mode::Rotate },
				{ "Scale",  EditorGizmo::Mode::Scale },
			};
			for (int i = 0; i < 3; ++i) {
				if (i > 0) ImGui::SameLine();
				bool active = m_gizmo.GetMode() == buttons[i].mode;
				if (active) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.85f, 1.0f));
				}
				if (ImGui::Button(buttons[i].label)) {
					m_gizmo.SetMode(buttons[i].mode);
				}
				if (active) {
					ImGui::PopStyleColor();
				}
			}
			ImGui::SameLine();
			// Toggle local/world space; label + highlight reflect the current state.
			bool local = m_gizmo.IsLocal();
			if (local) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.85f, 1.0f));
			}
			if (ImGui::Button(local ? "Local" : "World")) {
				m_gizmo.SetGizmoToLocal();
			}
			if (local) {
				ImGui::PopStyleColor();
			}
		}
		ImGui::End();

		RenderPlayBar();
		float deg;
		if (m_gizmo.IsRotating(deg)) {
			ImVec2 m = ImGui::GetMousePos();
			char buf[32];
			snprintf(buf, sizeof(buf), "%.1f\xC2\xB0", deg); // U+00B0 degree sign
			ImDrawList* dl = ImGui::GetForegroundDrawList();
			ImVec2 p{ m.x + 16.0f, m.y + 4.0f };
			ImU32 shadow = IM_COL32(0, 0, 0, 200);
			ImU32 white = IM_COL32(255, 255, 255, 255);
			for (int dx = -1; dx <= 1; ++dx)
				for (int dy = -1; dy <= 1; ++dy)
					if (dx || dy)
						dl->AddText(ImVec2(p.x + dx, p.y + dy), shadow, buf);
			dl->AddText(p, white, buf);
		}
	}

	void EditorState::RenderPlayBar() {
		const ImGuiViewport* vp = ImGui::GetMainViewport();
		ImVec2 origin = vp->WorkPos;
		float width = vp->WorkSize.x;
		if (ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(m_app.GetDockspaceId())) {
			origin = central->Pos;
			width = central->Size.x;
		}
		ImVec2 pos{ origin.x + width * 0.5f, origin.y + 8.0f };
		ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.0f)); // pivot: top-center
		ImGui::SetNextWindowViewport(vp->ID);
		ImGui::SetNextWindowBgAlpha(0.65f);
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav
			| ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDocking;

		if (ImGui::Begin("##PlayBar", nullptr, flags)) {
			const bool playing = (m_game != nullptr);
			if (playing) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.25f, 0.20f, 1.0f));
			}
			if (ImGui::Button(playing ? "Stop" : "Run Game")) {
				if (playing) {
					Logger::TraceLog(LOG_TRACE, "[Editor] Swap Editor mode");
					m_game->OnExit();
					m_game.reset();
					m_game = nullptr;
				}
				else {
					m_game = std::make_unique<Game>(m_app);
					Logger::TraceLog(LOG_TRACE, "[Game] Swap Game Playing mode");
					m_game->OnEnter();
					m_game->setCamera(m_gameCamera);
					m_game->copyhierarchy(m_scene);
					Logger::TraceLog(LOG_TRACE, "[Game] Copy hierarchy");
				}
			}
			if (playing) {
				ImGui::PopStyleColor();
			}
		}
		ImGui::End();
	}

	void EditorState::RenderUI() {
		// Menu bar.
		RenderMenuBar();
		// Floating gizmo-mode toolbar.
		RenderGizmoToolbar();
		// Each panel decides whether to draw based on its own open flag.
		RenderPanels();
	}
	void EditorState::OnExit()
	{
	}
}