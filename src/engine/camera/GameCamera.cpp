#include "GameCamera.hpp"
#include "GameCamera.hpp"
namespace Long {
	GameCamera::GameCamera()
	{
		static raylib::Vector3 default_target{ 0.0f,0.0f,0.0f }; 
		static const float fov = 45.0f; 
		m_camera.position = { 6.0f,12.0f,6.0f }; 
		m_camera.fovy = fov; 
		m_camera.projection = ::CAMERA_PERSPECTIVE;
		m_camera.SetTarget(default_target); 
		SetClip(0.3f, 50.0f);
	}
	void GameCamera::ApplyTransform(const raylib::Quaternion& quaternion, const raylib::Vector3& pos) {
		raylib::Vector3 right = raylib::Vector3(Vector3RotateByQuaternion({ 1, 0, 0 }, quaternion));
		raylib::Vector3 up = raylib::Vector3(Vector3RotateByQuaternion({ 0, 1, 0 }, quaternion));
		raylib::Vector3 forward = raylib::Vector3(Vector3RotateByQuaternion({ 0, 0, 1 }, quaternion));
		m_camera.SetPosition(pos);
		m_camera.SetTarget(Vector3Add(pos, forward));
		m_camera.SetUp(up);
	}
	void GameCamera::Update(float dt)
	{
	}
}