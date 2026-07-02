#pragma once
#ifndef _LIGHT_SYSTEM_HPP_
#define _LIGHT_SYSTEM_HPP_
#include <entt/entt.hpp>
#include <array>
#include <raylib-cpp.hpp>
namespace Long {
	struct GpuLight {
		raylib::Vector3 position{ 0, 0, 0 };
		raylib::Vector3 direction{ 0, -1, 0 };
		raylib::Vector4 color{ 1, 1, 1, 1 }; 
		float intensity{ 1.0f };
		int type{ 0 };                     
	};

	struct SceneLights {
		static constexpr int kMaxLights = 8;
		std::array<GpuLight, kMaxLights> lights{};
		int count{ 0 };
	};

	void LightSystem(entt::registry& registry, SceneLights& out);
}
#endif // !_LIGHT_SYSTEM_HPP_
