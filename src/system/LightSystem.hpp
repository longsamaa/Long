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
		float range{ 30.0f };
		//spotlight
		float innerCos{ 1.0f };
		float outerCos{ 1.0f };
		//point light 
		float constant;
		float linear;
		float quadratic;
		bool castsShadows{ false };
		int shadowIndex{ -1 };     // index into SceneLights::shadows (2D), or -1
		int cubeShadowIndex{ -1 }; // index into SceneLights::cubeShadows, or -1
	};

	struct ShadowCaster {
		raylib::Matrix lightViewProj{ MatrixIdentity() };
		unsigned int depthTexId{ 0 };
	};

	// A point light's omnidirectional shadow: a depth cubemap storing linear
	// distance-to-light, sampled by direction (fragPos - lightPos).
	struct CubeShadowCaster {
		unsigned int cubeTexId{ 0 };
		raylib::Vector3 lightPos{ 0, 0, 0 };
		float range{ 1.0f };
	};

	struct SceneLights {
		static constexpr int kMaxLights = 8;
		static constexpr int kMaxShadows = 4;
		static constexpr int kMaxCubeShadows = 2; // must match MAX_CUBE_SHADOWS in shaders
		std::array<LightParameter, kMaxLights> lights{};
		uint32_t size{ 0 };
		std::array<ShadowCaster, kMaxShadows> shadows{};
		uint32_t shadowCount{ 0 };
		std::array<CubeShadowCaster, kMaxCubeShadows> cubeShadows{};
		uint32_t cubeShadowCount{ 0 };

		// Hemisphere ambient for PBR, copied from the gradient skybox (Environment)
		// by ScenePass: up-facing normals receive the sky color, down-facing the
		// ground color -- cheap directional irradiance instead of a flat 0.15.
		raylib::Vector3 ambientSky{ 0.18f, 0.38f, 0.60f };
		raylib::Vector3 ambientGround{ 0.02f, 0.05f, 0.10f };
		// Keep ambient well below direct light (sky.b=0.6 at 1.0 would out-shine
		// an intensity-1 sun and flatten all shading).
		float ambientIntensity{ 0.4f };
	};

	void LightSystem(entt::registry& registry, SceneLights& out);
}
#endif // !_LIGHT_SYSTEM_HPP_