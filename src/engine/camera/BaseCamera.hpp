#pragma once
#ifndef _BASE_CAMERA_HPP_
#define _BASE_CAMERA_HPP_
namespace Long {
	class BaseCamera {
	public:
		virtual void Update(float dt) = 0;
		virtual void Begin3D() = 0;
		virtual void End3D() = 0;
	};
}
#endif // !_BASE_CAMERA_HPP_