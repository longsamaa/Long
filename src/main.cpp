#include "engine/Application.hpp"
#include "engine/Logger.hpp"
#include "editor/EditorState.hpp"
#include <memory>

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
