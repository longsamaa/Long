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

	// itamago's light/dark theme (imgui PR #511), ported to current ImGui: the
	// removed enums are mapped to their replacements (ChildWindowBg->ChildBg,
	// ComboBg->PopupBg, Column*->Separator*, ModalWindowDarkening->ModalWindowDimBg)
	// and the CloseButton* colors are dropped (the widget no longer exists). The
	// original's `<= ImGuiCol_COUNT` loop overran the array by one -- fixed to `<`.
	static void ApplyItamagoStyle(bool styleDark, float alpha) {
		ImGuiStyle& style = ImGui::GetStyle();
		style.Alpha = 1.0f;
		style.FrameRounding = 3.0f;
		style.Colors[ImGuiCol_Text]                 = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		style.Colors[ImGuiCol_WindowBg]             = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
		style.Colors[ImGuiCol_ChildBg]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_PopupBg]              = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
		style.Colors[ImGuiCol_Border]               = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
		style.Colors[ImGuiCol_BorderShadow]         = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
		style.Colors[ImGuiCol_FrameBg]              = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
		style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		style.Colors[ImGuiCol_TitleBg]              = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
		style.Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg]            = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
		style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
		style.Colors[ImGuiCol_CheckMark]            = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab]           = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
		style.Colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Button]               = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
		style.Colors[ImGuiCol_ButtonHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive]         = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Header]               = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		style.Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
		style.Colors[ImGuiCol_HeaderActive]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_Separator]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style.Colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
		style.Colors[ImGuiCol_SeparatorActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip]           = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
		style.Colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
		style.Colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
		style.Colors[ImGuiCol_PlotLines]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		style.Colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

		if (styleDark) {
			for (int i = 0; i < ImGuiCol_COUNT; i++) {
				ImVec4& col = style.Colors[i];
				float H, S, V;
				ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);
				if (S < 0.1f) {
					V = 1.0f - V;
				}
				ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
				if (col.w < 1.00f) {
					col.w *= alpha;
				}
			}
		}
		else {
			for (int i = 0; i < ImGuiCol_COUNT; i++) {
				ImVec4& col = style.Colors[i];
				if (col.w < 1.00f) {
					col.x *= alpha;
					col.y *= alpha;
					col.z *= alpha;
					col.w *= alpha;
				}
			}
		}
	}

	void Long::Application::SetEditorStyle(const EditorStyle& style)
	{
		// itamago theme is the primary style. Default = light variant, Dark = the
		// value-inverted dark variant. alpha=1.0 keeps translucent colors as-is.
		switch (style) {
		case EditorStyle::Default:
			ApplyItamagoStyle(/*styleDark*/ false, 1.0f);
			break;
		case EditorStyle::Dark:
		default:
			ApplyItamagoStyle(/*styleDark*/ true, 1.0f);
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