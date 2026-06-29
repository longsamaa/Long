#include "GameCamera.hpp"
#include "GameCamera.hpp"
namespace Long {
	GameCamera::GameCamera()
	{
		//create default camera 
		static raylib::Vector3 default_target{ 0.0f,0.0f,0.0f }; 
		m_camera.position = { 6.0f,6.0f,6.0f }; 
		m_camera.fovy = 45.0f; 
		m_camera.projection = ::CAMERA_PERSPECTIVE; 
		m_camera.SetTarget(default_target); 
	}
	void GameCamera::Update(float dt)
	{
	}
}