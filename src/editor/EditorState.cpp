#include "editor/EditorState.hpp"
#include "engine/Application.hpp"
#include "editor/panels/GpuInfoPanel.hpp"
#include "editor/panels/ProfilerPanel.hpp"
#include "editor/panels/ConsolePanel.hpp"
#include "editor/panels/HierarchyPanel.hpp"
#include "core/Components.hpp"
#include "system/RenderSystem.hpp"
#include "system/TransformSystem.hpp"
#include "system/RayCastSystem.hpp"
#include "engine/render/passes/ScenePass.hpp"
#include "engine/render/passes/CompositePass.hpp"
#include "imgui.h"
#include "raylib-cpp.hpp"

namespace Long {
	void EditorState::OnEnter() {
		m_panels.push_back(std::make_unique<GpuInfoPanel>());
		m_panels.push_back(std::make_unique<ProfilerPanel>(m_renderStats));
		m_panels.push_back(std::make_unique<ConsolePanel>());
		m_panels.push_back(std::make_unique<HierarchyPanel>(m_scene));

		// Build the render pipeline: scene -> screen. (Outline pass slots in
		// between these later.)
		m_renderer.AddPass(std::make_unique<ScenePass>());
		m_renderer.AddPass(std::make_unique<CompositePass>());

		testCreateDefaultCube();
	}

	void EditorState::Update(float dt) {
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

		// Cast a ray from the cursor and remember what it hit.
		raylib::Ray ray = ::GetScreenToWorldRay(GetMousePosition(), m_camera.Raw());
		m_hoverHit = RaycastSystem(m_scene.Registry(), ray);
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
				pivot = m_camera.Raw().target; // ray parallel to ground; keep target
			}
			m_camera.ZoomToward(wheel, pivot);
		}
	}

	void EditorState::RenderWorld() {
		RenderContext ctx;
		ctx.registry = &m_scene.Registry();
		ctx.assets   = &m_app.GetAssets();
		ctx.camera   = &m_camera.Raw();
		ctx.width    = (uint32_t)GetRenderWidth();
		ctx.height   = (uint32_t)GetRenderHeight();
		if (m_hoverHit.hit) {
			ctx.selectedEntities = { m_hoverHit.entity };
		}
		m_renderer.Render(ctx);
		m_renderStats = ctx.renderStats; // surface stats to the Profiler panel
	}

	void EditorState::testCreateDefaultCube()
	{
		auto& assets = m_app.GetAssets();
		raylib::Mesh cube = raylib::Mesh::Cube(1.0f, 1.0f, 1.0f);
		raylib::BoundingBox box = ::GetMeshBoundingBox(cube);
		uint32_t meshId = assets.AddMesh(std::move(cube));
		// material on top of it.
		uint32_t shaderId = assets.GetShaderId("default");
		uint32_t matId = assets.CreateDefaultMaterial(shaderId, raylib::Color::Maroon());
		auto& reg = m_scene.Registry();
		for (int i = 0; i < 3; ++i) {
			entt::entity e = m_scene.CreateEntity("cube");
			Transform t;
			t.position = { i * 2.0f - 2.0f, 0.5f, 0.0f };
			t.scale = { 1.0f, 1.0f, 1.0f };
			reg.emplace<Transform>(e, t);
			reg.emplace<MeshFilter>(e, MeshFilter{ meshId });
			reg.emplace<MeshRenderer>(e, MeshRenderer{ matId, raylib::Color::Maroon(), true });
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

	void EditorState::RenderUI() {
		// Menu bar.
		RenderMenuBar();
		// Each panel decides whether to draw based on its own open flag.
		RenderPanels();
	}
}