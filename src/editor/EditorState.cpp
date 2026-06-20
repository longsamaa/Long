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
#include "imgui.h"
#include "raylib-cpp.hpp"

namespace Long {
	void EditorState::OnEnter() {
		m_panels.push_back(std::make_unique<GpuInfoPanel>());
		m_panels.push_back(std::make_unique<ProfilerPanel>(m_renderStats));
		m_panels.push_back(std::make_unique<ConsolePanel>());
		m_panels.push_back(std::make_unique<HierarchyPanel>(m_scene));

		// Render pipeline order matters:
		//   Scene     -> sceneTarget (3D scene)
		//   Mask      -> maskTarget  (selected entities, flat white)
		//   Composite -> blit scene to screen
		//   Outline   -> edge-detect mask, draw outline over the screen
		m_renderer.AddPass(std::make_unique<ScenePass>());
		m_renderer.AddPass(std::make_unique<MaskPass>());
		m_renderer.AddPass(std::make_unique<CompositePass>());  // scene -> finalTarget
		m_renderer.AddPass(std::make_unique<OutlinePass>());    // outline -> finalTarget
		m_renderer.AddPass(std::make_unique<FXAAPass>());       // finalTarget -> screen (AA)

		m_environment.Init(m_app.GetAssets());
		m_panels.push_back(std::make_unique<EnvironmentPanel>(m_environment));
		testCreateDefaultCube();
	}

	void EditorState::Update(float dt) {
		m_commandQueue.Clear();
		if (!ImGui::GetIO().WantCaptureMouse) {
			m_camera.Update(dt);
		}
		TransformSystem(m_scene.Registry());
		UpdatePicking();
	}

	void EditorState::UpdatePicking() {
		// Don't pick/zoom while the cursor is over an ImGui panel.
		if (ImGui::GetIO().WantCaptureMouse) {
			m_hoverHit = RaycastHit{};
			return;
		}
		raylib::Ray ray = ::GetScreenToWorldRay(GetMousePosition(), m_camera.Raw());
		m_hoverHit = RaycastSystem(m_scene.Registry(), ray);
		if (::IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			if (m_hoverHit.hit) {
				m_selectedEntity = m_hoverHit.entity;
				auto& reg = m_scene.Registry();
				if (reg.all_of<Transform>(m_selectedEntity)) {
					m_camera.FocusOn(reg.get<Transform>(m_selectedEntity).position);
				}
			}
			else {
				m_selectedEntity = entt::null;
			}
		}
		float wheel = ::GetMouseWheelMove();
		if (wheel != 0.0f) {
			raylib::Vector3 pivot;
			if (m_hoverHit.hit) {
				pivot = m_hoverHit.point;
			}
			else if (fabsf(ray.direction.y) > 1e-5f) {
				float t = -ray.position.y / ray.direction.y;
				pivot = Vector3Add(ray.position, Vector3Scale(ray.direction, t > 0 ? t : 0));
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
		}
		m_renderer.Render(ctx);
		m_renderStats = ctx.renderStats;
	}

	void EditorState::testCreateDefaultCube()
	{
		auto& assets = m_app.GetAssets();
		raylib::Mesh cube = raylib::Mesh::Cube(1.0f, 1.0f, 1.0f);
		raylib::BoundingBox box = ::GetMeshBoundingBox(cube);
		uint32_t meshId = assets.AddMesh(std::move(cube));
		// material on top of it.
		uint32_t shaderId = assets.GetShaderId("default");
		// A few materials so the queue's sort-by-material actually has groups.
		uint32_t mat[3] = {
			assets.CreateDefaultMaterial(shaderId, raylib::Color::Maroon()),
			assets.CreateDefaultMaterial(shaderId, raylib::Color::RayWhite()),
			assets.CreateDefaultMaterial(shaderId, raylib::Color::Yellow()),
		};

		// Spawn a 10x10 grid of cubes (100 entities) to stress-test the pipeline.
		auto& reg = m_scene.Registry();
		const int N = 10;
		for (int x = 0; x < N; ++x) {
			for (int z = 0; z < N; ++z) {
				entt::entity e = m_scene.CreateEntity("cube");
				Transform t;
				t.position = { (x - N / 2) * 2.0f, 0.5f, (z - N / 2) * 2.0f };
				t.scale = { 1.0f, 1.0f, 1.0f };
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

	void EditorState::RenderUI() {
		// Menu bar.
		RenderMenuBar();
		// Each panel decides whether to draw based on its own open flag.
		RenderPanels();
	}
}