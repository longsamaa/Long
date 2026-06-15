#pragma once
#ifndef _LONG_APPLICATION_HPP_
#define _LONG_APPLICATION_HPP_
#include <memory>
#include <string>
#include "raylib-cpp.hpp"
namespace Long {
	class AppState;
	//Application manages main loop, active state
	class Application {
	public:

		enum class EditorStyle {
			Default,
			Dark
		};

		struct Config {
			int width = 1280;
			int height = 800;
			std::string title = "long";
			int targetFps = 144;
			bool high_dpi = true;
			EditorStyle editorMode = EditorStyle::Dark;
		};
		explicit Application(const Config& config = {});
		~Application();
		// No copy
		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;
		//style
		void SetEditorStyle(const EditorStyle& style);
		void SetState(std::unique_ptr<AppState> state);
		// Blocks until the window is closed or Quit() is called.
		void Run();
		// Request the loop to stop after the current frame.
		void Quit() { m_running = false; }
		raylib::Window& GetWindow() { return m_window; }

	private:
		Config m_config;
		raylib::Window m_window;
		std::unique_ptr<AppState> m_state;
		bool m_running = true;
	};
}
#endif // !_LONG_APPLICATION_HPP_
