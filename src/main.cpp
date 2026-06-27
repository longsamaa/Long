#include "engine/Application.hpp"
#include "engine/Logger.hpp"
#include "editor/EditorState.hpp"
#include <memory>

// Ask laptop GPU drivers to run this app on the discrete GPU instead of the
// integrated one. The drivers look for these exact exported symbols:
//   NvOptimusEnablement      -> NVIDIA Optimus
//   AmdPowerXpressRequestHighPerformance -> AMD switchable graphics
// Must be exported from the .exe (extern "C" + dllexport).
#if defined(_WIN32)
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main() {
    // Install before creating the window so we capture raylib's init logs too.
    Long::Logger::Install();

    Long::Application::Config config;
    config.title = "long - tiny 3d engine";
    config.width = 1920;   // Full HD
    config.height = 1080;
    Long::Application app(config);
    app.SetState(std::make_unique<Long::EditorState>(app));
    app.Run();
    return 0;
}
