#include "core/Scene.hpp"
#include "core/Components.hpp"
#include <unordered_map>
namespace Long {
	static void markTransformDirty(entt::registry& reg, entt::entity e) {
		++reg.get<Transform>(e).version;
		reg.emplace_or_replace<DirtyTransform>(e);
	}

	Scene::Scene() {
		m_registry.on_construct<Transform>().connect<&markTransformDirty>();
		m_registry.on_update<Transform>().connect<&markTransformDirty>();
	}

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

	void Scene::SetDirty(entt::entity e) {
		if (m_registry.valid(e)) {
			m_registry.emplace_or_replace<DirtyTransform>(e);
		}
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
		copy_component.operator() < Transform > ();  
		copy_component.operator() < MatrixTransform > ();
		copy_component.operator() < WorldAABB > ();
		copy_component.operator() < MeshFilter > ();
		copy_component.operator() < MeshRenderer > ();
		copy_component.operator() < BoxCollider3D > ();
		for (auto [e, h] : src.view<Hierarchy>().each()) {
			Hierarchy copy;
			copy.parent = (h.parent == entt::null) ? entt::null : map_entt[h.parent];
			copy.children.reserve(h.children.size());
			for (entt::entity child : h.children) {
				copy.children.push_back(map_entt[child]);
			}
			m_registry.emplace<Hierarchy>(map_entt[e], std::move(copy));
		}
	}
} // namespace Long