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

	// Translation (position) stored in a matrix's last column.
	static raylib::Vector3 MatrixTranslation(const raylib::Matrix& m) {
		return { m.m12, m.m13, m.m14 };
	}

	// Decompose an affine matrix into a Transform (position, rotation, scale).
	// Used to read a WORLD transform back out of world_matrix for the gizmo, and to
	// convert an edited world transform into a child's local space.
	static Transform DecomposeToTransform(const raylib::Matrix& m) {
		Transform t;
		t.position = { m.m12, m.m13, m.m14 };
		// Scale = length of each basis column.
		raylib::Vector3 cx{ m.m0, m.m1, m.m2 };
		raylib::Vector3 cy{ m.m4, m.m5, m.m6 };
		raylib::Vector3 cz{ m.m8, m.m9, m.m10 };
		t.scale = { cx.Length(), cy.Length(), cz.Length() };
		// Remove scale to get a pure rotation matrix, then to a quaternion.
		raylib::Matrix rot = m;
		if (t.scale.x != 0) { rot.m0 /= t.scale.x; rot.m1 /= t.scale.x; rot.m2 /= t.scale.x; }
		if (t.scale.y != 0) { rot.m4 /= t.scale.y; rot.m5 /= t.scale.y; rot.m6 /= t.scale.y; }
		if (t.scale.z != 0) { rot.m8 /= t.scale.z; rot.m9 /= t.scale.z; rot.m10 /= t.scale.z; }
		t.quaternion = raylib::Quaternion(QuaternionNormalize(QuaternionFromMatrix(rot)));
		return t;
	}

	static void TransformAABB(const raylib::Vector3& localMin, const raylib::Vector3& localMax,
		const raylib::Matrix& m, raylib::Vector3& outMin, raylib::Vector3& outMax)
	{
		outMin = { m.m12, m.m13, m.m14 };
		outMax = { m.m12, m.m13, m.m14 };

		const float rot[3][3] = {
			{ m.m0, m.m4, m.m8  },
			{ m.m1, m.m5, m.m9  },
			{ m.m2, m.m6, m.m10 },
		};
		const float lmin[3] = { localMin.x, localMin.y, localMin.z };
		const float lmax[3] = { localMax.x, localMax.y, localMax.z };
		float* omin[3] = { &outMin.x, &outMin.y, &outMin.z };
		float* omax[3] = { &outMax.x, &outMax.y, &outMax.z };

		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				float a = rot[i][j] * lmin[j];
				float b = rot[i][j] * lmax[j];
				*omin[i] += (a < b) ? a : b;
				*omax[i] += (a < b) ? b : a;
			}
		}
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

	static raylib::Vector3 KelvinToRGB(float kelvin)
	{
		kelvin = std::clamp(kelvin, 1000.0f, 40000.0f);
		float temp = kelvin / 100.0f;
		float r, g, b;
		// Red
		if (temp <= 66.0f)
		{
			r = 255.0f;
		}
		else
		{
			r = 329.698727446f * std::pow(temp - 60.0f, -0.1332047592f);
			r = std::clamp(r, 0.0f, 255.0f);
		}
		// Green
		if (temp <= 66.0f)
		{
			g = 99.4708025861f * std::log(temp) - 161.1195681661f;
		}
		else
		{
			g = 288.1221695283f * std::pow(temp - 60.0f, -0.0755148492f);
		}
		g = std::clamp(g, 0.0f, 255.0f);
		// Blue
		if (temp >= 66.0f)
		{
			b = 255.0f;
		}
		else if (temp <= 19.0f)
		{
			b = 0.0f;
		}
		else
		{
			b = 138.5177312231f * std::log(temp - 10.0f) - 305.0447927307f;
			b = std::clamp(b, 0.0f, 255.0f);
		}
		return {
			r / 255.0f,
			g / 255.0f,
			b / 255.0f
		};
	}
}
#endif // !_MATH_HPP_