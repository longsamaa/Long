#include "engine/Application.hpp"
#include "editor/EditorState.hpp"
#include <memory>

int main() {
    Long::Application::Config config;
    config.title = "long - tiny 3d engine"; 
    Long::Application app(config);
    app.SetState(std::make_unique<Long::EditorState>(app));
    app.Run();
    return 0;
}
