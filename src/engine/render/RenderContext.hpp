#pragma once
#ifndef _RENDER_CONTEXT_HPP_
#define _RENDER_CONTEXT_HPP_
#include <entt/entt.hpp>
#include <raylib-cpp.hpp>
#include "system/RenderStats.hpp"
#include "RenderTarget.hpp"
namespace Long {
	entt::registry* registry{ nullptr };
	raylib::Camera3D* camera{ nullptr };
	std::vector<entt::entity> selectedEntities = {};
	uint32_t width{ 0 };
	uint32_t height{ 0 };
	//Rendertarget
	RenderStats renderStats;
}
#endif // !_RENDER_CONTEXT_HPP_