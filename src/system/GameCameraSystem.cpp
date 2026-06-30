#include "GameCameraSystem.hpp"
#include "core/components.hpp"
namespace Long {
	void GameCameraSystem(entt::registry& registry, BaseCamera& game_camera)
	{
		auto view = registry.view<MainCamera, Transform>();
		for (entt::entity e : view) {
			const auto& [camera,transform]  = view.get<MainCamera, Transform>(e);
			if (camera.buildFromTransformVersion != transform.getVersion()) {
				game_camera.ApplyTransform(transform.getQuaternion(), transform.getPos());
				camera.buildFromTransformVersion = transform.getVersion(); 
			}
		}
	}
}