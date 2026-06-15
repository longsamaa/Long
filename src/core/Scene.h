#pragma once

#include <entt/entt.hpp>
#include <string>

namespace Long {

// Owns the ECS registry and is the single entry point for creating/destroying
// entities. Systems read the registry directly via Registry().
class Scene {
public:
    // Create an entity, optionally with a Name component.
    entt::entity CreateEntity(const std::string& name = "");

    void DestroyEntity(entt::entity e);

    entt::registry& Registry() { return m_registry; }
    const entt::registry& Registry() const { return m_registry; }

    // Remove all entities.
    void Clear() { m_registry.clear(); }

private:
    entt::registry m_registry;
};

} // namespace Long
