#pragma once
#include <entt/entt.hpp>
#include <string>
namespace Long {
class Scene {
public:
    // Create an entity, optionally with a Name component.
    entt::entity CreateEntity(const std::string& name = "");
    void DestroyEntity(entt::entity e);
    entt::registry& Registry() { return m_registry; }
    const entt::registry& Registry() const { return m_registry; }
    void Clear() { m_registry.clear(); }
    std::string getSceneName() { return m_sceneName;  };
private:
    entt::registry m_registry;
    std::string m_sceneName{ "SampleScene" }; 
};

} // namespace Long
