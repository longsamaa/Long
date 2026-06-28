#pragma once
#ifndef _MATH_HPP_
#define _MATH_HPP_
#include <raylib-cpp.hpp>
#include "core/Components.hpp"
namespace Long {
	static raylib::Matrix LocalMatrix(const Transform& t) {
		const raylib::Vector3& scale = t.getScale();
		const raylib::Quaternion& quaternion = t.getQuaternion();
		const raylib::Vector3& position = t.getPos();
		raylib::Matrix s = raylib::Matrix::Scale(scale.x, scale.y, scale.z);
		raylib::Matrix r = QuaternionToMatrix(quaternion);
		raylib::Matrix tr = raylib::Matrix::Translate(position.x, position.y, position.z);
		return s * r * tr;
	}
}
#endif // !_MATH_HPP_