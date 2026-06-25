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
#include "engine/render/passes/ScenePass.hpp"
#include "engine/render/passes/MaskPass.hpp"
#include "engine/render/passes/CompositePass.hpp"
#include "engine/render/passes/OutlinePass.hpp"
#include "engine/render/passes/FXAAPass.hpp"
#include "engine/render/passes/GizmoPass.hpp"
#include "imgui.h"
#include "imgui_internal.h" // DockBuilderGetCentralNode
#include "raylib-cpp.hpp"
#include <chrono>

namespace Long {
	void EditorState::OnEnter() {
		m_panels.push_back(std::make_unique<GpuInfoPanel>());
		m_panels.push_back(std::make_unique<ProfilerPanel>(m_renderStats));
		m_panels.push_back(std::make_unique<ConsolePanel>());
		m_panels.push_back(std::make_unique<HierarchyPanel>(m_scene));

		m_renderer.AddPass(std::make_unique<ScenePass>());
		m_renderer.AddPass(std::make_unique<MaskPass>());
		m_renderer.AddPass(std::make_unique<CompositePass>());  // scene -> finalTarget
		m_renderer.AddPass(std::make_unique<OutlinePass>());    // outline -> finalTarget
		m_renderer.AddPass(std::make_unique<FXAAPass>());       // finalTarget -> screen (AA)
		m_renderer.AddPass(std::make_unique<GizmoPass>());      // gizmo overlay on screen

		m_environment.Init(m_app.GetAssets());
		m_panels.push_back(std::make_unique<EnvironmentPanel>(m_environment));
		testCreateDefaultCube();
	}

	void EditorState::Update(float dt) {
		using Clock = std::chrono::high_resolution_clock;
		auto ms = [](Clock::time_point from) {
			return std::chrono::duration<double, std::milli>(Clock::now() - from).count();
		};
		const auto tUpdate = Clock::now();

		m_commandQueue.Clear();
		if (!ImGui::GetIO().WantCaptureMouse) {
			m_camera.Update(dt);
		}
		auto t0 = Clock::now();
		TransformSystem(m_scene.Registry());
		m_msTransformSystem = ms(t0);

		bool gizmoHasInput = false;
		if (m_selectedEntity != entt::null && m_scene.Registry().valid(m_selectedEntity)
			&& m_scene.Registry().all_of<Transform>(m_selectedEntity)
			&& !ImGui::GetIO().WantCaptureMouse) {
			m_gizmo.Update(m_camera.Raw(), m_scene.Registry().get<Transform>(m_selectedEntity));
			gizmoHasInput = m_gizmo.IsActive() || m_gizmo.IsHot();
		}

		m_msPicking = 0.0;
		if (!gizmoHasInput) {
			UpdatePicking();
		}
		m_msUpdate = ms(tUpdate);
	}

	void EditorState::UpdatePicking() {
		// Don't pick/zoom while the cursor is over an ImGui panel.
		if (ImGui::GetIO().WantCaptureMouse) {
			m_hoverHit = RaycastHit{};
			return;
		}
		if (raylib::Mouse::IsButtonDown(MOUSE_BUTTON_LEFT)) {
			raylib::Ray ray = ::GetScreenToWorldRay(GetMousePosition(), m_camera.Raw());
			auto tPick = std::chrono::high_resolution_clock::now();
			m_hoverHit = RaycastSystem(m_scene.Registry(), ray);
			m_msPicking = std::chrono::duration<double, std::milli>(
				std::chrono::high_resolution_clock::now() - tPick).count();
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
		// Merge in the Update-phase timings measured before Render() (which Reset()s
		// the stats), so the Profiler shows scene + update stages together.
		m_renderStats.msTransformSystem = m_msTransformSystem;
		m_renderStats.msPicking = m_msPicking;
		m_renderStats.msUpdate = m_msUpdate;
		m_renderStats.msEndDrawing = m_msEndDrawing; // from the previous frame
	}

	void EditorState::testCreateDefaultCube()
	{
		auto& assets = m_app.GetAssets();
		raylib::Mesh cube = raylib::Mesh::Cube(1.0f, 1.0f, 1.0f);
		raylib::BoundingBox box(cube); 
		uint32_t meshId = assets.AddMesh(std::move(cube));
		// material on top of it.
		uint32_t wireId = assets.GetShaderId("wireframe");
		// Wireframe material: light-brown edge line on a slightly darker face so
		// the brown border still reads against the fill.
		// Signature: CreateWireframeMaterial(shader, lineColor, faceColor, thickness)
		raylib::Color lightBrown{ 196, 164, 132, 255 }; // edge line
		raylib::Color faceBrown{ 120, 96, 72, 255 };    // darker fill behind it
		uint32_t mat[3] = {
			assets.CreateWireframeMaterial(wireId, lightBrown, faceBrown, 0.01f),
			assets.CreateWireframeMaterial(wireId, lightBrown, faceBrown, 0.01f),
			assets.CreateWireframeMaterial(wireId, lightBrown, faceBrown, 0.01f),
		};
		// Spawn a 10x10 grid of cubes (100 entities) to stress-test the pipeline.
		// Cubes are 1 unit wide, so a spacing of 1.0 makes them sit edge-to-edge.
		auto& reg = m_scene.Registry();
		const int N = 500;
		const float spacing = 1.0f;
		for (int x = 0; x < N; ++x) {
			for (int z = 0; z < N; ++z) {
				entt::entity e = m_scene.CreateEntity("cube");
				Transform t;
				t.setPos({ (x - N / 2) * spacing, 0.5f, (z - N / 2) * spacing });
				t.setScale({ 1.0f, 1.0f, 1.0f });
				reg.emplace<Transform>(e, t);
				reg.emplace<MeshFilter>(e, MeshFilter{ meshId });
				// Cycle materials so sort-by-material has something to group.
				reg.emplace<MeshRenderer>(e, MeshRenderer{ mat[(x + z) % 3], raylib::Color::White(), true });
				reg.emplace<BoxCollider3D>(e, BoxCollider3D{ box });
			}
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