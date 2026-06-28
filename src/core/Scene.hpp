#pragma once
#include <entt/entt.hpp>
#include <string>
#include "engine/camera/BaseCamera.hpp"
namespace Long {
	class Scene {
	public:
		Scene();
		// Create an entity, optionally with a Name component.
		entt::entity CreateEntity(const std::string& name = "");
		void DestroyEntity(entt::entity e);
		entt::registry& Registry() { return m_registry; }
		const entt::registry& Registry() const { return m_registry; }
		void Clear() { m_registry.clear(); }
		std::string getSceneName() { return m_sceneName; };
		void Clone(const Scene& anotherScene);
		// Tag an entity so TransformSystem recomputes its (and its children's) world
		// matrix this frame. Call after creating or moving an entity's Transform.
		void SetDirty(entt::entity e);
	private:
		entt::registry m_registry;
		std::string m_sceneName{ "SampleScene" };
	};
} // namespace Long