#include "engine/Application.hpp"
#include "engine/AppState.hpp"

#include "rlImGui.h"
#include "imgui.h"
#include <filesystem>

// Member init list constructs m_window, which opens the window (InitWindow).
// Native resolution, resizable window (good for an editor). Do NOT use
// FLAG_WINDOW_HIGHDPI -- it upscales a low-res framebuffer.
namespace Long {
	Application::Application(const Config& config)
		: m_config(config),
		m_window(config.width, config.height, config.title,
			FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI) {
		m_window.SetTargetFPS(m_config.targetFps);
		rlImGuiSetup(true);
#ifdef IMGUI_HAS_DOCK
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
		SetEditorStyle(m_config.editorMode);
		std::filesystem::path shaderDir =
			std::filesystem::path(GetApplicationDirectory()) / "shaders";
		m_assets.LoadAllShaders(shaderDir);
		// Instanced copies for DrawMeshInstanced batches.
		m_assets.LoadInstancedVariant(shaderDir, "default");
		m_assets.LoadInstancedVariant(shaderDir, "wireframe");
	}

	Application::~Application() {
		if (m_state) {
			m_state->OnExit();
			m_state.reset();
		}
		rlImGuiShutdown();
	}

	void Long::Application::SetEditorStyle(const EditorStyle& style)
	{
		switch (style) {
		case EditorStyle::Default:
			ImGui::StyleColorsClassic();
			break;
		case EditorStyle::Dark:
			ImGui::StyleColorsDark();
			break;
		default:
			ImGui::StyleColorsDark();
			break;
		}
	}

	void Application::SetState(std::unique_ptr<AppState> state) {
		if (m_state) {
			m_state->OnExit();
		}
		m_state = std::move(state);
		if (m_state) {
			m_state->OnEnter();
		}
	}

	void Application::Run() {
		while (m_running && !m_window.ShouldClose()) {
			const float dt = m_window.GetFrameTime();

			if (m_state) {
				m_state->Update(dt);
			}
			m_window.BeginDrawing();
			m_window.ClearBackground(raylib::Color::White());
			if (m_state) {
				m_state->RenderWorld();
				rlImGuiBegin();
#ifdef IMGUI_HAS_DOCK
				m_dockspaceId = ImGui::DockSpaceOverViewport(0, NULL, ImGuiDockNodeFlags_PassthruCentralNode);
#endif
				m_state->RenderUI();
				rlImGuiEnd();
			}
			m_window.EndDrawing();
		}
	}
}