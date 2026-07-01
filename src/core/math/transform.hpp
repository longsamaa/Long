#pragma once
#ifndef _MATH_HPP_
#define _MATH_HPP_
#include <raylib-cpp.hpp>
#include "core/Components.hpp"
#include "engine/camera/BaseCamera.hpp"
namespace Long {
	static raylib::Matrix LocalMatrix(const Transform& t) {
		const raylib::Vector3& scale = t.scale;
		const raylib::Quaternion& quaternion = t.quaternion;
		const raylib::Vector3& position = t.position;
		raylib::Matrix s = raylib::Matrix::Scale(scale.x, scale.y, scale.z);
		raylib::Matrix r = QuaternionToMatrix(quaternion);
		raylib::Matrix tr = raylib::Matrix::Translate(position.x, position.y, position.z);
		return s * r * tr;
	}
	static raylib::Quaternion CameraToQuaternion(const BaseCamera& camera) {
		const raylib::Vector3& pos = camera.Raw().GetPosition();
		const raylib::Vector3& cam_up = camera.Raw().GetUp();
		const raylib::Vector3& target = camera.Raw().GetTarget();
		
		raylib::Vector3 forward = pos.Subtract(target).Normalize();
		raylib::Vector3 right = forward.CrossProduct(cam_up).Normalize();
		
		Vector3 up = Vector3CrossProduct(right, forward);
		raylib::Matrix m = raylib::Matrix::Identity();
		m.m0 = right.x;
		m.m1 = right.y;
		m.m2 = right.z;

		m.m4 = up.x;
		m.m5 = up.y;
		m.m6 = up.z;

		m.m8 = -forward.x;
		m.m9 = -forward.y;
		m.m10 = -forward.z;
		Quaternion q = QuaternionNormalize(QuaternionFromMatrix(m));
		return raylib::Quaternion(q); 
	}
}
#endif // !_MATH_HPP_