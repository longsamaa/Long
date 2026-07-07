#include "engine/Application.hpp"
#include "engine/AppState.hpp"
#include "engine/GpuInfo.hpp"
#include "engine/Logger.hpp"
#include "rlImGui.h"
#include "imgui.h"
#include "helpers/TimerHelper.hpp"
#include <filesystem>
#include <iostream>

namespace Long {
	Application::Application(const Config& config)
		: m_config(config),
		m_window(config.width, config.height, config.title,
			FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE) {
		//m_window.SetTargetFPS(m_config.targetFps);
		{
			int mon = GetCurrentMonitor();
			int mw = GetMonitorWidth(mon);
			int mh = GetMonitorHeight(mon);
			int w = config.width, h = config.height;
			if (mw > 0 && w > mw - 80) w = mw - 80;   // leave room for taskbar/borders
			if (mh > 0 && h > mh - 120) h = mh - 120;
			if (w != config.width || h != config.height) {
				m_window.SetSize(w, h);
			}
			m_window.SetPosition((mw - w) / 2, (mh - h) / 2);
		}
		GpuInfo gpu = GpuInfo::Query();
		Logger::TraceLog(LOG_INFO, "=== Init diagnostics ===");
		Logger::TraceLog(LOG_INFO, std::format("GPU vendor   : {}", gpu.vendor.c_str()));
		Logger::TraceLog(LOG_INFO, std::format("GPU renderer : {}", gpu.renderer.c_str()));
		Logger::TraceLog(LOG_INFO, std::format("GL version   : {}", gpu.version.c_str()));
		Logger::TraceLog(LOG_INFO, std::format("Window size  : {} {}  (render {} {})",
			GetScreenWidth(), GetScreenHeight(), GetRenderWidth(), GetRenderHeight()));
		Logger::TraceLog(LOG_INFO, std::format("App dir      : {}", GetApplicationDirectory()));

		rlImGuiSetup(true);
#ifdef IMGUI_HAS_DOCK
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
		SetEditorStyle(m_config.editorMode);
		std::filesystem::path shaderDir =
			std::filesystem::path(GetApplicationDirectory()) / "shaders";
		Logger::TraceLog(LOG_INFO, std::format("Loading shaders from: {}", shaderDir.string().c_str()));
		m_assets.LoadAllShaders(shaderDir);

		m_assets.LoadInstancedVariant(shaderDir, "default");
		m_assets.LoadInstancedVariant(shaderDir, "wireframe");
		m_assets.LoadInstancedVariant(shaderDir, "emissive");
		m_assets.LoadInstancedVariant(shaderDir, "shadow_depth");
		m_assets.LoadInstancedVariant(shaderDir, "shadow_depth_point");
		m_assets.LoadInstancedVariant(shaderDir, "pbr");
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
			Logger::TraceLog(LOG_INFO, "AppState::OnEnter() begin");
			m_state->OnEnter();
			Logger::TraceLog(LOG_INFO, "AppState::OnEnter() done");
		}
	}

	void Application::Run() {
		TraceLog(LOG_INFO, "Application::Run() loop start");
		while (m_running && !m_window.ShouldClose()) {
			const auto tFrame = Time::now();   // measure the WHOLE frame
			const float dt = m_window.GetFrameTime();
			double msUpdate = 0.0;
			if (m_state) {
				m_state->Update(dt);
			}

			m_window.BeginDrawing();
			m_window.ClearBackground(raylib::Color::White());
			if (m_state) {
				m_state->BeginFrame(); 
				m_state->RenderWorld();
				m_state->EndFrame(); 
				rlImGuiBegin();
#ifdef IMGUI_HAS_DOCK
				m_dockspaceId = ImGui::DockSpaceOverViewport(0, NULL, ImGuiDockNodeFlags_PassthruCentralNode);
#endif
				m_state->RenderUI();
				rlImGuiEnd();
			}

			m_window.EndDrawing();             // SwapBuffers + waits for vsync
		}
		TraceLog(LOG_INFO, "Application::Run() loop exit");
	}
}