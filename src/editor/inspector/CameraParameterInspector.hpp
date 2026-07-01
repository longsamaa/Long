#pragma once
#ifndef _CAMERA_PARAMETER_INSPECTOR_HPP_
#define _CAMERA_PARAMETER_INSPECTOR_HPP_
namespace Long {
	struct GameCameraParameter;
	class CameraParameterInspector {
	public:
		static bool Draw(GameCameraParameter& param);
	};
}
#endif // !_CAMERA_PARAMETER_INSPECTOR_HPP_
