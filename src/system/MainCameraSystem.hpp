#pragma once 
#ifndef _MAIN_CAMERA_SYSTEM_HPP_
#define _MAIN_CAMERA_SYSTEM_HPP_
#include <entt/entt.hpp>
#include <raylib-cpp.hpp>
namespace Long {
	void MainCameraSystem(entt::registry& registry, raylib::Camera3D& main_camera);
}
#endif // !_MAIN_CAMERA_SYSTEM_HPP_
