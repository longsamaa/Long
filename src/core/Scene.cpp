#include "core/Scene.hpp"
#include "core/Components.hpp"
#include <unordered_map>
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
	void Scene::Clone(const Scene& anotherScene)
	{
		m_registry.clear();
		const entt::registry& src = anotherScene.Registry();
		std::map<entt::entity, entt::entity> map_entt; //src - target
		for (entt::entity e : *src.storage<entt::entity>()) {
			map_entt[e] = m_registry.create();
		}
		auto copy_component = [&]<typename T>()
		{
			auto view = src.view<T>();
			for (entt::entity e : src.view<T>()) {
				const auto& value = view.get<T>(e);
				m_registry.emplace<T>(map_entt[e], value);
			}
		};
		copy_component.operator() < Name > ();
		copy_component.operator() < MatrixTransform > ();
		copy_component.operator() < WorldAABB > ();
		copy_component.operator() < MeshFilter > ();
		copy_component.operator() < MeshRenderer > ();
		copy_component.operator() < BoxCollider3D > ();
		copy_component.operator() < Hierarchy > ();
	}
} // namespace Long