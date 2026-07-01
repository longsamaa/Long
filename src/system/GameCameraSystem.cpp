#include "GameCameraSystem.hpp"
#include "core/components.hpp"
namespace Long {
	void GameCameraSystem(entt::registry& registry, BaseCamera& game_camera)
	{
		auto view = registry.view<MainCamera, Transform, GameCameraParameter>();
		for (entt::entity e : view) {
			const auto& [camera,transform, parameter]  = view.get<MainCamera, Transform, GameCameraParameter>(e);
			if (camera.buildFromTransformVersion != transform.version) {
				game_camera.ApplyTransform(transform.quaternion, transform.position);
				camera.buildFromTransformVersion = transform.version; 
			}
			if (camera.buildFromCameraParameterVersion != parameter.version) {
				game_camera.ApplyParameter(parameter.projection,
					parameter.fov,
					parameter.near,
					parameter.far); 
				camera.buildFromCameraParameterVersion = parameter.version; 
			}
		}
	}
}