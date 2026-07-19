#include "LightSystem.hpp"
#include "core/Components.hpp"
#include "core/math/transform.hpp" // KelvinToRGB
#include <cmath>

namespace Long {
	void LightSystem(entt::registry& registry, SceneLights& out) {
		out.size = 0;
		auto view = registry.view<MatrixTransform, LightComponent>();
		for (entt::entity e : view) {
			if (out.size >= SceneLights::kMaxLights) {
				break;
			}
			auto [mt, lc] = view.get<MatrixTransform, LightComponent>(e);
			const raylib::Matrix& w = mt.world_matrix;

			raylib::Vector3 position{ w.m12, w.m13, w.m14 };
			const raylib::Vector3& d = lc.direction;
			lc.world_direction = raylib::Vector3{
				w.m0 * d.x + w.m4 * d.y + w.m8 * d.z,
				w.m1 * d.x + w.m5 * d.y + w.m9 * d.z,
				w.m2 * d.x + w.m6 * d.y + w.m10 * d.z
			}.Normalize();
			lc.buildFromTransformVersion = mt.buildFromTransformVersion;
			LightParameter& g = out.lights[out.size++];
			g.source = e;
			g.position = position;
			g.direction = lc.world_direction;
			raylib::Vector3 kelvin = KelvinToRGB(lc.temperature);
			g.color = { lc.color.r / 255.0f * kelvin.x,
						lc.color.g / 255.0f * kelvin.y,
						lc.color.b / 255.0f * kelvin.z,
						lc.color.a / 255.0f };
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