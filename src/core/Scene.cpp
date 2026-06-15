#include "core/Scene.h"
#include "core/Components.h"

namespace Long {

entt::entity Scene::CreateEntity(const std::string& name) {
    entt::entity e = m_registry.create();
    if (!name.empty()) {
        m_registry.emplace<Name>(e, name);
    }
    return e;
}

void Scene::DestroyEntity(entt::entity e) {
    m_registry.destroy(e);
}

} // namespace Long
