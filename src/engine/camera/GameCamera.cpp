#include "GameCamera.hpp"
#include "GameCamera.hpp"
namespace Long {
	GameCamera::GameCamera()
	{
		static raylib::Vector3 default_target{ 0.0f,0.0f,0.0f }; 
		static const float fov = 45.0f; 
		m_camera.position = { 6.0f,6.0f,6.0f }; 
		m_camera.fovy = fov; 
		m_camera.projection = ::CAMERA_ORTHOGRAPHIC;
		m_camera.SetTarget(default_target); 
		SetClip(0.3f, 50.0f);
	}
	void GameCamera::Update(float dt)
	{
	}
}