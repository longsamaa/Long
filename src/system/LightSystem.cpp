#include "LightSystem.hpp"
#include "core/Components.hpp"

namespace Long {
	void LightSystem(entt::registry& registry, SceneLights& out) {
		out.count = 0;
		auto view = registry.view<Transform, LightComponent>();
		for (entt::entity e : view) {
			if (out.count >= SceneLights::kMaxLights) {
				break;
			}
			auto [t, lc] = view.get<Transform, LightComponent>(e); 
			if (lc.buildFromTransformVersion != t.version) {
				lc.world_direction = raylib::Vector3(lc.direction)
					.RotateByQuaternion(t.quaternion)
					.Normalize();
				lc.buildFromTransformVersion = t.version;
			}
			GpuLight& g = out.lights[out.count++];
			g.position = t.position;
			g.direction = lc.world_direction;
			g.color = { lc.color.r / 255.0f, lc.color.g / 255.0f,
						lc.color.b / 255.0f, lc.color.a / 255.0f };
			g.intensity = lc.intensity;
			g.type = (int)lc.type;
		}
	}
}
