#include "engine/Application.h"
#include "editor/EditorState.h"
#include <memory>

int main() {
    Long::Application::Config config;
    config.title = "long - tiny 3d engine"; 
    Long::Application app(config);
    app.SetState(std::make_unique<Long::EditorState>(app));
    app.Run();
    return 0;
}
