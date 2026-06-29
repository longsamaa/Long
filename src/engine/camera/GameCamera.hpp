#pragma once
#ifndef _MAIN_CAMERA_HPP_
#define _MAIN_CAMERA_HPP_
#include "BaseCamera.hpp"
#include <raylib-cpp.hpp>
namespace Long {
	class GameCamera : public BaseCamera {
	public:
		GameCamera();
		~GameCamera() = default;
	public:
		const raylib::Camera3D& Raw() const override { return m_camera; }
		void Update(float dt) override;
		void BeginMode() override { ApplyClip(); m_camera.BeginMode(); }
		void EndMode() override { m_camera.EndMode(); }
	private:
		raylib::Camera3D m_camera;
	};
}
#endif // !_MAIN_CAMERA_HPP_