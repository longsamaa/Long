#include "LightSystem.hpp"
#include "core/Components.hpp"
#include <cmath>

namespace Long {
	void LightSystem(entt::registry& registry, SceneLights& out) {
		out.size = 0;
		auto view = registry.view<Transform, LightComponent>();
		for (entt::entity e : view) {
			if (out.size >= SceneLights::kMaxLights) {
				break;
			}
			auto [t, lc] = view.get<Transform, LightComponent>(e);
			if (lc.buildFromTransformVersion != t.version) {
				lc.world_direction = raylib::Vector3(lc.direction)
					.RotateByQuaternion(t.quaternion)
					.Normalize();
				lc.buildFromTransformVersion = t.version;
			}
			LightParameter& g = out.lights[out.size++];
			g.source = e;
			g.position = t.position;
			g.direction = lc.world_direction;
			g.color = { lc.color.r / 255.0f, lc.color.g / 255.0f,
						lc.color.b / 255.0f, lc.color.a / 255.0f };
			g.intensity = lc.intensity;
			g.type = (uint32_t)lc.type;
			//Spot light 
			
			float inner = lc.innerAngle;
			float outer = (lc.outerAngle > inner) ? lc.outerAngle : inner + 0.01f;
			g.innerCos = cosf(inner * DEG2RAD);
			g.outerCos = cosf(outer * DEG2RAD);
			g.range = (lc.range > 0.01f) ? lc.range : 0.01f;
			
			
			//Point light 
			
			
			g.constant = lc.constant; 
			g.linear = lc.linear; 
			g.quadratic = lc.quadratic; 
			


			g.castsShadows = lc.castsShadows;
			g.shadowIndex = -1; // assigned by ShadowPass
		}
	}
}