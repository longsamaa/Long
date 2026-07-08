#pragma once
#ifndef _LONG_MATH_RAYLIB_HPP_
#define _LONG_MATH_RAYLIB_HPP_

// raylib/raymath math backend. The types are TYPEDEFS of raylib's own structs,
// not wrappers -- so Math::Vec3 IS ::Vector3. That means:
//   * no conversion when passing values in or out (same type, same layout),
//   * Math:: values drop straight into any remaining raylib/rlgl call,
//   * every function below is `inline` + forwards, so at -O2 the Math:: layer
//     disappears entirely (identical codegen to calling raymath directly).
//
// Include <raymath.h> for the C math functions; raylib.h (via raylib-cpp) for
// the ::Vector3 / ::Matrix / ::Quaternion struct definitions.

#include "raylib.h"
#include "raymath.h"

namespace Long::Math {

	// ---- Types (typedefs -> zero-cost) ----
	using Vec2 = ::Vector2;
	using Vec3 = ::Vector3;
	using Vec4 = ::Vector4;
	using Mat4 = ::Matrix;
	using Quat = ::Quaternion;

	// =====================================================================
	//  Vec3  (12 functions)
	// =====================================================================
	inline Vec3 Add(Vec3 a, Vec3 b)            { return Vector3Add(a, b); }
	inline Vec3 Sub(Vec3 a, Vec3 b)            { return Vector3Subtract(a, b); }
	inline Vec3 Scale(Vec3 v, float s)         { return Vector3Scale(v, s); }
	inline Vec3 Normalize(Vec3 v)              { return Vector3Normalize(v); }
	inline float Dot(Vec3 a, Vec3 b)           { return Vector3DotProduct(a, b); }
	inline Vec3 Cross(Vec3 a, Vec3 b)          { return Vector3CrossProduct(a, b); }
	inline float Length(Vec3 v)                { return Vector3Length(v); }
	inline float LengthSqr(Vec3 v)             { return Vector3LengthSqr(v); }
	inline Vec3 Transform(Vec3 v, Mat4 m)      { return Vector3Transform(v, m); }
	inline Vec3 Min(Vec3 a, Vec3 b)            { return Vector3Min(a, b); }
	inline Vec3 Max(Vec3 a, Vec3 b)            { return Vector3Max(a, b); }
	inline Vec3 RotateByQuat(Vec3 v, Quat q)   { return Vector3RotateByQuaternion(v, q); }

	// -- Extra Vec3 helpers commonly needed alongside the 12 above --
	inline Vec3 Negate(Vec3 v)                 { return Vector3Negate(v); }
	inline Vec3 Lerp(Vec3 a, Vec3 b, float t)  { return Vector3Lerp(a, b, t); }

	// =====================================================================
	//  Mat4  (9 functions)
	//  CONVENTION (must be matched by every backend, e.g. GLM):
	//    Mul(a, b) applies `a` FIRST, then `b` -- i.e. for a child/local `a`
	//    and parent/world `b`, world = Mul(local, parent). raylib's column-major
	//    MatrixMultiply(left, right) already does left-first, so we forward
	//    directly. LookAt/Perspective/Ortho: raylib is right-handed with a
	//    GL-style [-1,1] depth range; GLM must be configured to match.
	// =====================================================================
	inline Mat4 Identity()                     { return MatrixIdentity(); }
	inline Mat4 Mul(Mat4 a, Mat4 b)            { return MatrixMultiply(a, b); }
	inline Mat4 Invert(Mat4 m)                 { return MatrixInvert(m); }
	inline Mat4 Transpose(Mat4 m)              { return MatrixTranspose(m); }
	inline Mat4 Translate(Vec3 v)              { return MatrixTranslate(v.x, v.y, v.z); }
	inline Mat4 ScaleMat(Vec3 v)               { return MatrixScale(v.x, v.y, v.z); }
	inline Mat4 LookAt(Vec3 eye, Vec3 target, Vec3 up) { return MatrixLookAt(eye, target, up); }
	inline Mat4 Perspective(float fovy, float aspect, float n, float f) {
		return MatrixPerspective(fovy, aspect, n, f);
	}
	inline Mat4 Ortho(float l, float r, float b, float t, float n, float f) {
		return MatrixOrtho(l, r, b, t, n, f);
	}

	// =====================================================================
	//  Quat  (7 functions)
	//  QuatFromEuler/ToEuler use raylib's (pitch=x, yaw=y, roll=z) order.
	// =====================================================================
	inline Mat4 QuatToMatrix(Quat q)           { return QuaternionToMatrix(q); }
	inline Quat QuatFromMatrix(Mat4 m)         { return QuaternionFromMatrix(m); }
	inline Quat QuatNormalize(Quat q)          { return QuaternionNormalize(q); }
	inline Quat QuatFromEuler(float x, float y, float z) { return QuaternionFromEuler(x, y, z); }
	inline Vec3 QuatToEuler(Quat q)            { return QuaternionToEuler(q); }
	inline Quat QuatFromAxisAngle(Vec3 axis, float angle) { return QuaternionFromAxisAngle(axis, angle); }
	inline Quat QuatMul(Quat a, Quat b)        { return QuaternionMultiply(a, b); }

	// =====================================================================
	//  Bridge / element access  (4 functions)
	//  These wrap the parts where raylib's field layout (.m0..m15) is exposed,
	//  so callers never touch .mN directly (GLM indexes as m[col][row]).
	// =====================================================================

	// Column-major float[16] for GPU upload. raylib's MatrixToFloatV returns a
	// float16 by value; we stash it in a thread_local so the returned pointer
	// stays valid until the next ToFloatPtr call (upload immediately).
	inline const float* ToFloatPtr(const Mat4& m) {
		static thread_local float16 buf;
		buf = MatrixToFloatV(m);
		return buf.v;
	}

	// Translation = last column (m12,m13,m14).
	inline Vec3 Translation(const Mat4& m)     { return { m.m12, m.m13, m.m14 }; }

	// Build a matrix from three basis axes + a position (column layout):
	//   right -> column 0, up -> column 1, fwd -> column 2, pos -> column 3.
	inline Mat4 FromBasis(Vec3 right, Vec3 up, Vec3 fwd, Vec3 pos) {
		Mat4 m = MatrixIdentity();
		m.m0 = right.x; m.m1 = right.y; m.m2 = right.z;
		m.m4 = up.x;    m.m5 = up.y;    m.m6 = up.z;
		m.m8 = fwd.x;   m.m9 = fwd.y;   m.m10 = fwd.z;
		m.m12 = pos.x;  m.m13 = pos.y;  m.m14 = pos.z;
		return m;
	}

	// Element at (col, row) -- raylib stores column-major, so index = col*4 + row.
	inline float At(const Mat4& m, int col, int row) {
		return (&m.m0)[col * 4 + row];
	}

	// =====================================================================
	//  Composite  (3 functions)  -- built on the element access above.
	// =====================================================================

	// Decompose an affine matrix into position / rotation / scale.
	inline void Decompose(const Mat4& m, Vec3& pos, Quat& rot, Vec3& scale) {
		pos = { m.m12, m.m13, m.m14 };
		Vec3 cx{ m.m0, m.m1, m.m2 };
		Vec3 cy{ m.m4, m.m5, m.m6 };
		Vec3 cz{ m.m8, m.m9, m.m10 };
		scale = { Vector3Length(cx), Vector3Length(cy), Vector3Length(cz) };
		Mat4 r = m;
		if (scale.x != 0) { r.m0 /= scale.x; r.m1 /= scale.x; r.m2 /= scale.x; }
		if (scale.y != 0) { r.m4 /= scale.y; r.m5 /= scale.y; r.m6 /= scale.y; }
		if (scale.z != 0) { r.m8 /= scale.z; r.m9 /= scale.z; r.m10 /= scale.z; }
		rot = QuaternionNormalize(QuaternionFromMatrix(r));
	}

	// Transform a local AABB by `m`, producing the world-space AABB (min/max).
	inline void TransformAABB(Vec3 lmin, Vec3 lmax, const Mat4& m, Vec3& omin, Vec3& omax) {
		omin = { m.m12, m.m13, m.m14 };
		omax = { m.m12, m.m13, m.m14 };
		const float rot[3][3] = {
			{ m.m0, m.m4, m.m8  },
			{ m.m1, m.m5, m.m9  },
			{ m.m2, m.m6, m.m10 },
		};
		const float lo[3] = { lmin.x, lmin.y, lmin.z };
		const float hi[3] = { lmax.x, lmax.y, lmax.z };
		float* pmin[3] = { &omin.x, &omin.y, &omin.z };
		float* pmax[3] = { &omax.x, &omax.y, &omax.z };
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				float a = rot[i][j] * lo[j];
				float b = rot[i][j] * hi[j];
				*pmin[i] += (a < b) ? a : b;
				*pmax[i] += (a < b) ? b : a;
			}
		}
	}

	// Orientation-only quaternion that looks along `forward` with `up` (the
	// camera-basis convention used by CameraToQuaternion: -forward on Z).
	inline Quat LookRotation(Vec3 forward, Vec3 up) {
		Vec3 f = Vector3Normalize(forward);
		Vec3 right = Vector3Normalize(Vector3CrossProduct(f, up));
		Vec3 u = Vector3CrossProduct(right, f);
		Mat4 m = MatrixIdentity();
		m.m0 = right.x; m.m1 = right.y; m.m2 = right.z;
		m.m4 = u.x;     m.m5 = u.y;     m.m6 = u.z;
		m.m8 = -f.x;    m.m9 = -f.y;    m.m10 = -f.z;
		return QuaternionNormalize(QuaternionFromMatrix(m));
	}

} // namespace Long::Math

#endif // !_LONG_MATH_RAYLIB_HPP_
