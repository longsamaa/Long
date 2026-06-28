#pragma once
#ifndef _FRUSTUM_CULLING_HPP_
#define _FRUSTUM_CULLING_HPP_
#include <raylib-cpp.hpp>
#include <array>
#include "core/math/Frustum.hpp"
namespace Long {
	class FrustumCulling {
	public: 
		FrustumCulling() = default; 
		~FrustumCulling() = default; 
	public: 
		void setCamera(raylib::Camera3D* _camera);
		void update();
		bool isVisible(const raylib::Vector3& min, const raylib::Vector3& max) const;
		const std::array<Plane, 6>& planes() const { return m_planes; }
	private:
		void buildPlane();
	private:
		raylib::Camera3D* camera;
		std::array<Plane, 6> m_planes;
	};
}
#endif // !_FRUSTUM_CULLING_HPP_
