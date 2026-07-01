#pragma once
#ifndef _BASE_CAMERA_HPP_
#define _BASE_CAMERA_HPP_
#include <raylib-cpp.hpp>
#include "rlgl.h"   // rlSetClipPlanes
namespace Long {
	class BaseCamera {
	public:
		virtual ~BaseCamera() = default;
		virtual void Update(float dt) = 0;
		virtual void BeginMode() = 0;
		virtual void EndMode() = 0;
		virtual const raylib::Camera3D& Raw() const = 0;
		float Near() const { return m_near; }
		float Far()  const { return m_far; }
		void SetClip(float n, float f) { m_near = n; m_far = f; }
		virtual void ApplyTransform(const raylib::Quaternion& quaternion,
			const raylib::Vector3& pos) = 0; 
		virtual void ApplyParameter(const uint32_t& projection,
			const float& fov,
			const float& near, 
			const float& far) = 0;
	protected:
		void ApplyClip() const { rlSetClipPlanes(m_near, m_far); }
		float m_near{ 0.1f };
		float m_far{ 1000.0f };
	};
}
#endif // !_BASE_CAMERA_HPP_