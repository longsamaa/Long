#include "core/Scene.hpp"
#include "core/Components.hpp"
#include "engine/Application.hpp"
#include "engine/AssetManager.hpp"
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

	entt::entity Scene::CreateObject(const CreateObjectType& type)
	{
		switch (type)
		{
		case CreateObjectType::GameObject:
		{
			Transform transform;
			static const std::string game_object_name = "GameObject"; 
			entt::entity e = m_registry.create();
			m_registry.emplace<Name>(e, game_object_name);
			m_registry.emplace<Transform>(e, transform);
			return e; 
		}
		case CreateObjectType::Cube:
		{
			auto& assets = m_app->GetAssets(); 
			static const std::string name = "Cube";
			entt::entity cube = m_registry.create();
			m_registry.emplace<Name>(cube, name);
			raylib::Mesh cubeMesh = raylib::Mesh::Cube(1.0f, 1.0f, 1.0f);
			raylib::BoundingBox box_collider(cubeMesh);
			uint32_t meshId = assets.AddMesh(std::move(cubeMesh));
			uint32_t pbrId = assets.GetShaderId("pbr");
			uint32_t mat = assets.CreateDefaultMaterial(pbrId, raylib::Color::White());
			m_registry.emplace<Transform>(cube, Transform{});
			m_registry.emplace<MeshFilter>(cube, MeshFilter{ meshId });
			m_registry.emplace<MeshRenderer>(cube, MeshRenderer{ mat, raylib::Color::White(), true });
			m_registry.emplace<BoxCollider3D>(cube, BoxCollider3D{ box_collider });
			m_registry.emplace<Hierarchy>(cube, Hierarchy{ entt::null, {} });
			return cube; 
		}
		case CreateObjectType::Sphere: {
			auto& assets = m_app->GetAssets();
			static const std::string name = "Sphere";
			entt::entity sphere = m_registry.create();
			m_registry.emplace<Name>(sphere, name);
			raylib::Mesh sphereMesh = raylib::Mesh::Sphere(1,16,16);
			raylib::BoundingBox box_collider(sphereMesh);
			uint32_t meshId = assets.AddMesh(std::move(sphereMesh));
			uint32_t pbrId = assets.GetShaderId("pbr");
			uint32_t mat = assets.CreateDefaultMaterial(pbrId, raylib::Color::White());
			m_registry.emplace<Transform>(sphere, Transform{});
			m_registry.emplace<MeshFilter>(sphere, MeshFilter{ meshId });
			m_registry.emplace<MeshRenderer>(sphere, MeshRenderer{ mat, raylib::Color::White(), true });
			m_registry.emplace<BoxCollider3D>(sphere, BoxCollider3D{ box_collider });
			m_registry.emplace<Hierarchy>(sphere, Hierarchy{ entt::null, {} });
			return sphere;
		}
		case CreateObjectType::Cylinder: {
			auto& assets = m_app->GetAssets();
			static const std::string name = "Cylinder";
			entt::entity cylinder = m_registry.create();
			m_registry.emplace<Name>(cylinder, name);
			raylib::Mesh cylinderMesh = raylib::Mesh::Cylinder(1, 1, 16);
			raylib::BoundingBox box_collider(cylinderMesh);
			uint32_t meshId = assets.AddMesh(std::move(cylinderMesh));
			uint32_t pbrId = assets.GetShaderId("pbr");
			uint32_t mat = assets.CreateDefaultMaterial(pbrId, raylib::Color::White());
			m_registry.emplace<Transform>(cylinder, Transform{});
			m_registry.emplace<MeshFilter>(cylinder, MeshFilter{ meshId });
			m_registry.emplace<MeshRenderer>(cylinder, MeshRenderer{ mat, raylib::Color::White(), true });
			m_registry.emplace<BoxCollider3D>(cylinder, BoxCollider3D{ box_collider });
			m_registry.emplace<Hierarchy>(cylinder, Hierarchy{ entt::null, {} });
			return cylinder;
		}
		default: 
			return entt::null; 
		}
		return entt::null;
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