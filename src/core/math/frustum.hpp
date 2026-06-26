#pragma once
#ifndef _FRUSTUM_HPP_
#define _FRUSTUM_HPP_
#include <raylib-cpp.hpp>
namespace Long {
	struct Plane {
		raylib::Vector3 normal; 
		float d; 
	};
}
#endif // !_FRUSTUM_HPP_
