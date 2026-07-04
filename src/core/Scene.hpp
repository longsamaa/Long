#pragma once
#include <entt/entt.hpp>
#include <string>
#include "engine/camera/BaseCamera.hpp"
namespace Long {
	class Application; // fwd-declared: core must not include engine headers

	enum class CreateObjectType : uint32_t {
		GameObject = 0,
		Cube = 1, 
		Sphere = 2,
		Cylinder = 3
	};

	class Scene {
	public:
		Scene();
		entt::entity CreateObject(const CreateObjectType& type); 
		entt::entity CreateEntity(const std::string& name = "");
		void DestroyEntity(entt::entity e);
		entt::registry& Registry() { return m_registry; }
		const entt::registry& Registry() const { return m_registry; }
		void Clear() { m_registry.clear(); }
		std::string getSceneName() { return m_sceneName; };
		void Clone(const Scene& anotherScene);
		void SetDirty(entt::entity e);
		// Owning Application (assets, config, ...). Set once by the state that
		// owns the scene; may be null in unit-style contexts, so check it.
		void SetApplication(Application* app) { m_app = app; }
		Application* GetApplication() const { return m_app; }
	private:
		entt::registry m_registry;
		Application* m_app{ nullptr };
		std::string m_sceneName{ "SampleScene" };
	};
} // namespace Long