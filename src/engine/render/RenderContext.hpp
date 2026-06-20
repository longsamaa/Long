#pragma once
#ifndef _RENDER_CONTEXT_HPP_
#define _RENDER_CONTEXT_HPP_
#include <entt/entt.hpp>
#include <raylib-cpp.hpp>
#include <vector>
#include <any>
#include "system/RenderStats.hpp"
#include "RenderTarget.hpp"
#include "CommandQueue.hpp"
namespace Long {
	class AssetManager;
	struct RenderContext {
		CommandQueue* commandQueue{ nullptr };
		entt::registry* registry{ nullptr };
		AssetManager*   assets{ nullptr };     // ScenePass needs meshes/materials
		raylib::Camera3D* camera{ nullptr };
		uint32_t width{ 0 };
		uint32_t height{ 0 };
		RenderTarget* sceneTarget{ nullptr };  // 3D scene color
		RenderTarget* maskTarget{ nullptr };   // selected-only mask (outline)
		RenderStats renderStats;
	};
}
#endif // !_RENDER_CONTEXT_HPP_