#pragma once
#ifndef _LIGHT_SYSTEM_HPP_
#define _LIGHT_SYSTEM_HPP_
#include <entt/entt.hpp>
#include <array>
#include <raylib-cpp.hpp>
namespace Long {
	//Light paramater de nap vo context 
	struct LightParameter {
		entt::entity source{ entt::null };
		raylib::Vector3 position{ 0, 0, 0 };
		raylib::Vector3 direction{ 0, -1, 0 };
		raylib::Vector4 color{ 1, 1, 1, 1 };
		float intensity{ 1.0f };
		uint32_t type{ 0 };
	};

	struct SceneLights {
		static constexpr int kMaxLights = 8;
		std::array<LightParameter, kMaxLights> lights{};
		uint32_t size{ 0 };

		// Shadow mapping inputs, filled by ShadowPass (not LightSystem). Bound to
		// the scene shader alongside the lights so a single struct carries all the
		// lighting a draw needs. depthTexId == 0 disables shadow sampling.
		raylib::Matrix lightViewProj{ MatrixIdentity() }; // world -> light clip space
		unsigned int shadowMapTexId{ 0 };
		uint32_t shadowMapSize{ 0 };  // resolution (square), for debug blit
		bool shadowsEnabled{ false };
	};

	void LightSystem(entt::registry& registry, SceneLights& out);
}
#endif // !_LIGHT_SYSTEM_HPP_
