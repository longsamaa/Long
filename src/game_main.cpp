#include "engine/Application.hpp"
#include "engine/Logger.hpp"
#include "game/Game.hpp"
#include <memory>

// Entry point for the SHIPPED GAME (LongGame.exe): no editor, no ImGui tools --
// it just boots straight into the Game runtime state. Same engine core as the
// editor, different front-end. This is also the shape an Android launcher takes.
#if defined(_WIN32)
extern "C" {
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main() {
	Long::Logger::Install();
	Long::Application::Config config;
	config.title = "Long Game";
	config.width = 1280;
	config.height = 720;
	Long::Application app(config);
	app.SetState(std::make_unique<Long::Game>(app));
	app.Run();
	return 0;
}