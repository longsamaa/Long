#pragma once
#ifndef _GAME_CAMERA_SYSTEM_HPP_
#define _GAME_CAMERA_SYSTEM_HPP_
#include <entt/entt.hpp>
#include "engine/camera/BaseCamera.hpp"
namespace Long {
	//camera update 
	void GameCameraSystem(entt::registry& registry, BaseCamera& game_camera);
}
#endif // !_GAME_CAMERA_SYSTEM_HPP_
