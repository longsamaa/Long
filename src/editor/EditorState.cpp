#include "editor/EditorState.hpp"
#include "engine/Application.hpp"
#include "editor/panels/GpuInfoPanel.hpp"
#include "editor/panels/ProfilerPanel.hpp"
#include "editor/panels/ConsolePanel.hpp"
#include "editor/panels/HierarchyPanel.hpp"
#include "editor/panels/EnvironmentPanel.hpp"
#include "core/Components.hpp"
#include "system/RenderSystem.hpp"
#include "system/TransformSystem.hpp"
#include "system/RayCastSystem.hpp"
#include "system/WorldBoundSystem.hpp"
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
#include "imgui.h"
#include "imgui_internal.h" // DockBuilderGetCentralNode
#include "raylib-cpp.hpp"
#include "helpers/TimerHelper.hpp"
#include "engine/Logger.hpp"
namespace Long {
	void EditorState::OnEnter() {
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: begin, creating panels");
		m_panels.push_back(std::make_unique<GpuInfoPanel>());
		m_panels.push_back(std::make_unique<ProfilerPanel>(m_renderStats));
		m_panels.push_back(std::make_unique<ConsolePanel>());
		m_panels.push_back(std::make_unique<HierarchyPanel>(m_scene));
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: panels done, adding render passes");
		m_renderer.AddPass(std::make_unique<ScenePass>());          // scene -> sceneTarget (HDR)
		m_renderer.AddPass(std::make_unique<MaskPass>());           // selection mask
		m_renderer.AddPass(std::make_unique<BrightPass>());         // sceneTarget -> brightTarget
		m_renderer.AddPass(std::make_unique<BloomPass>());          // brightTarget -> blurTarget (mip-chain bloom)
		m_renderer.AddPass(std::make_unique<BloomCompositePass>()); // scene+bloom -> finalTarget (HDR)
		m_renderer.AddPass(std::make_unique<TonemapPass>());        // HDR -> screen (tonemap + FXAA)
		m_renderer.AddPass(std::make_unique<OutlinePass>());        // overlay, straight to screen
		m_renderer.AddPass(std::make_unique<GizmoPass>());          // overlay, straight to screen
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: passes added, init environment");
		m_environment.Init(m_app.GetAssets());
		m_panels.push_back(std::make_unique<EnvironmentPanel>(m_environment));
		//init camera
		m_frustum.setCamera(&m_camera.Raw());
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: createGround begin");
		createGround();
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: createGround done, createEmissiveBoxes begin");
		createEmissiveBoxes();
		Logger::TraceLog(LOG_INFO, "[Editor] OnEnter: createEmissiveBoxes done");
	}

	void EditorState::Update(float dt) {
		const auto tUpdate = Time::now();
		m_commandQueue.Clear();
		if (!ImGui::GetIO().WantCaptureMouse) {
			m_camera.Update(dt);
		}
		auto t0 = Time::now();
		TransformSystem(m_scene.Registry());
		WorldBoundsSystem(m_scene.Registry(),m_app.GetAssets());
		m_msTransformSystem = Time::elapsedSecond(t0);
		bool gizmoHasInput = false;
		if (m_selectedEntity != entt::null && m_scene.Registry().valid(m_selectedEntity)
			&& m_scene.Registry().all_of<Transform>(m_selectedEntity)
			&& !ImGui::GetIO().WantCaptureMouse) {
			m_gizmo.Update(m_camera.Raw(), m_scene.Registry().get<Transform>(m_selectedEntity));
			gizmoHasInput = m_gizmo.IsActive() || m_gizmo.IsHot();
		}
		if (!gizmoHasInput) {
			UpdatePicking();
		}
		m_msUpdate = Time::elapsedSecond(tUpdate);
		m_frustum.update(); 
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

	void EditorState::RenderWorld() {
		RenderContext ctx;
		ctx.commandQueue = &m_commandQueue;
		ctx.commandDebugQueue = &m_commandDebugQueue;
		ctx.environment = &m_environment;
		ctx.registry = &m_scene.Registry();
		ctx.assets = &m_app.GetAssets();
		ctx.camera = &m_camera.Raw();
		ctx.frustum = &m_frustum;
		ctx.width = (uint32_t)GetRenderWidth();
		ctx.height = (uint32_t)GetRenderHeight();
		if (m_selectedEntity != entt::null && m_scene.Registry().valid(m_selectedEntity)) {
			ctx.selectedEntities = { m_selectedEntity };
			auto& reg = m_scene.Registry();
			if (reg.all_of<Transform>(m_selectedEntity)) {
				ctx.gizmo = &m_gizmo;
				ctx.gizmoTarget = &reg.get<Transform>(m_selectedEntity);
			}
		}
		m_renderer.Render(ctx);
		m_renderStats = ctx.renderStats;
		m_renderStats.msTransformSystem = m_msTransformSystem;
		m_renderStats.msUpdate = m_msUpdate;
	}

	void EditorState::createGround()
	{
		auto& assets = m_app.GetAssets();
		auto& reg = m_scene.Registry();
		raylib::Mesh cube = raylib::Mesh::Cube(1.0f, 1.0f, 1.0f);
		raylib::BoundingBox box(cube);
		uint32_t meshId = assets.AddMesh(std::move(cube));
		uint32_t wireId = assets.GetShaderId("wireframe");
		raylib::Color lightBrown{ 196, 164, 132, 255 };
		raylib::Color faceBrown{ 120, 96, 72, 255 };
		uint32_t mat = assets.CreateWireframeMaterial(wireId, lightBrown, faceBrown, 0.02f);
		entt::entity parent = m_scene.CreateEntity("ground");
		reg.emplace<Transform>(parent, Transform{});
		std::vector<entt::entity> children;
		const int N = 100;
		const float spacing = 1.0f;
		for (int x = 0; x < N; ++x) {
			for (int z = 0; z < N; ++z) {
				entt::entity tile = m_scene.CreateEntity("tile");
				Transform t;
				t.setPos({ (x - N / 2) * spacing, -0.5f, (z - N / 2) * spacing });
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
			t.setPos(g.pos);
			t.setScale({ 0.5f, 0.5f, 0.5f }); // bigger so the glow reads
			reg.emplace<Transform>(e, t);
			reg.emplace<MeshFilter>(e, MeshFilter{ meshId });
			reg.emplace<MeshRenderer>(e, MeshRenderer{ emat, raylib::Color::White(), true });
			reg.emplace<BoxCollider3D>(e, BoxCollider3D{ box });
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

	void EditorState::RenderUI() {
		// Menu bar.
		RenderMenuBar();
		// Floating gizmo-mode toolbar.
		RenderGizmoToolbar();
		// Each panel decides whether to draw based on its own open flag.
		RenderPanels();
	}
}