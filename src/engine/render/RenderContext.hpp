#pragma once
#ifndef _RENDER_CONTEXT_HPP_
#define _RENDER_CONTEXT_HPP_
#include <entt/entt.hpp>
#include <raylib-cpp.hpp>
#include <vector>
#include "system/RenderStats.hpp"
#include "RenderTarget.hpp"
namespace Long {
	class AssetManager;

	struct RenderContext {
		// Inputs (set by the owner each frame).
		entt::registry* registry{ nullptr };
		AssetManager*   assets{ nullptr };     // ScenePass needs meshes/materials
		raylib::Camera3D* camera{ nullptr };
		std::vector<entt::entity> selectedEntities = {};
		uint32_t width{ 0 };
		uint32_t height{ 0 };

		// Shared targets (owned by the Renderer; passes read/write).
		RenderTarget* sceneTarget{ nullptr };  // 3D scene color
		RenderTarget* maskTarget{ nullptr };   // selected-only mask (outline)

		// Output accumulated by passes.
		RenderStats renderStats;
	};
}
#endif // !_RENDER_CONTEXT_HPP_