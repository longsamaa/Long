#pragma once
#ifndef _LONG_APPLICATION_HPP_
#define _LONG_APPLICATION_HPP_
#include <memory>
#include <string>
#include "raylib-cpp.hpp"
#include "engine/AssetManager.hpp"
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
		void Run();
		void Quit() { m_running = false; }
		raylib::Window& GetWindow() { return m_window; }
		AssetManager& GetAssets() { return m_assets; }
		// Dockspace id of the main viewport (valid after the first frame); lets
		// states anchor overlays to the central (scene) node.
		unsigned int GetDockspaceId() const { return m_dockspaceId; }

	private:
		Config m_config;
		raylib::Window m_window;
		AssetManager m_assets;
		std::unique_ptr<AppState> m_state;
		bool m_running = true;
		unsigned int m_dockspaceId = 0;
	};
}
#endif // !_LONG_APPLICATION_HPP_
