#pragma once
#ifndef _MAIN_CAMERA_HPP_
#define _MAIN_CAMERA_HPP_
#include "BaseCamera.hpp"
#include <raylib-cpp.hpp>
namespace Long {
	class MainCamera : public BaseCamera {
	public:
		MainCamera() = default;
		~MainCamera() = default;
	public:
		raylib::Camera3D& Raw() { return m_camera; }
	public:
		void Update(float dt) override;
		void Begin3D() override { m_camera.BeginMode(); };
		void End3D() override { m_camera.EndMode(); };
		raylib::Camera3D m_camera;
	};
}
#endif // !_MAIN_CAMERA_HPP_