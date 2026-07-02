#include "helpers/draw_debug_helper.hpp"
#include "raymath.h"
#include "rlgl.h"
#include <array>
namespace Long {
	raylib::BoundingBox MakeWorldBoundingBox(const raylib::BoundingBox& localBox,
		const raylib::Matrix& world) {
		const Vector3& mn = localBox.min;
		const Vector3& mx = localBox.max;

		// The 8 corners of the local box.
		Vector3 corners[8] = {
			{ mn.x, mn.y, mn.z },
			{ mx.x, mn.y, mn.z },
			{ mn.x, mx.y, mn.z },
			{ mn.x, mn.y, mx.z },
			{ mx.x, mx.y, mn.z },
			{ mx.x, mn.y, mx.z },
			{ mn.x, mx.y, mx.z },
			{ mx.x, mx.y, mx.z },
		};

		// Transform every corner to world space and take the enclosing AABB.
		Vector3 worldMin = Vector3Transform(corners[0], world);
		Vector3 worldMax = worldMin;
		for (int i = 1; i < 8; ++i) {
			Vector3 w = Vector3Transform(corners[i], world);
			worldMin = Vector3Min(worldMin, w);
			worldMax = Vector3Max(worldMax, w);
		}

		return raylib::BoundingBox(worldMin, worldMax);
	}

	void DrawCircle3D(raylib::Vector3 center,
		raylib::Vector3 u,
		raylib::Vector3 v,
		float radius,
		raylib::Color color,
		float thickness, 
		int segments) {
		rlSetLineWidth(thickness);
		raylib::Vector3 prev = center.Add(u.Scale(radius));
		for (int s = 1; s <= segments; ++s) {
			float a = (float)s / segments * 2.0f * PI;
			raylib::Vector3 p = center
				.Add(u.Scale(cosf(a) * radius))
				.Add(v.Scale(sinf(a) * radius));
			p.DrawLine3D(prev, color);
			prev = p;
		}
		rlSetLineWidth(1.0f); // restore default
	}

	void DrawRing3D(raylib::Vector3 center, 
		raylib::Vector3 u,
		raylib::Vector3 v,
		float radius, float width, raylib::Color color, int segments) {
		float inner = radius - width * 0.5f;
		float outer = radius + width * 0.5f;
		rlBegin(RL_TRIANGLES);
		rlColor4ub(color.r, color.g, color.b, color.a);
		for (int s = 0; s < segments; ++s) {
			float a0 = (float)s / segments * 2.0f * PI;
			float a1 = (float)(s + 1) / segments * 2.0f * PI;
			Vector3 dir0 = Vector3Add(Vector3Scale(u, cosf(a0)), Vector3Scale(v, sinf(a0)));
			Vector3 dir1 = Vector3Add(Vector3Scale(u, cosf(a1)), Vector3Scale(v, sinf(a1)));
			Vector3 i0 = Vector3Add(center, Vector3Scale(dir0, inner));
			Vector3 o0 = Vector3Add(center, Vector3Scale(dir0, outer));
			Vector3 i1 = Vector3Add(center, Vector3Scale(dir1, inner));
			Vector3 o1 = Vector3Add(center, Vector3Scale(dir1, outer));
			rlVertex3f(i0.x, i0.y, i0.z); rlVertex3f(o0.x, o0.y, o0.z); rlVertex3f(o1.x, o1.y, o1.z);
			rlVertex3f(i0.x, i0.y, i0.z); rlVertex3f(o1.x, o1.y, o1.z); rlVertex3f(i1.x, i1.y, i1.z);
		}
		rlEnd();
	}

	void DrawTorus3D(raylib::Vector3 center,
		raylib::Vector3 u, 
		raylib::Vector3 v,
		float radius,
		float tube, 
		raylib::Color color,
		int ringSegments,
		int tubeSegments) {
		Vector3 n = Vector3Normalize(Vector3CrossProduct(u, v));
		auto point = [&](int i, int j) -> Vector3 {
			float a = (float)i / ringSegments * 2.0f * PI; // around the main ring
			float b = (float)j / tubeSegments * 2.0f * PI; // around the tube
			Vector3 radial = Vector3Add(Vector3Scale(u, cosf(a)), Vector3Scale(v, sinf(a)));
			Vector3 ringPos = Vector3Add(center, Vector3Scale(radial, radius));
			Vector3 off = Vector3Add(Vector3Scale(radial, cosf(b) * tube),
				Vector3Scale(n, sinf(b) * tube));
			return Vector3Add(ringPos, off);
			};
		rlBegin(RL_TRIANGLES);
		rlColor4ub(color.r, color.g, color.b, color.a);
		for (int i = 0; i < ringSegments; ++i) {
			for (int j = 0; j < tubeSegments; ++j) {
				Vector3 p00 = point(i, j);
				Vector3 p10 = point(i + 1, j);
				Vector3 p01 = point(i, j + 1);
				Vector3 p11 = point(i + 1, j + 1);
				rlVertex3f(p00.x, p00.y, p00.z); rlVertex3f(p10.x, p10.y, p10.z); rlVertex3f(p11.x, p11.y, p11.z);
				rlVertex3f(p00.x, p00.y, p00.z); rlVertex3f(p11.x, p11.y, p11.z); rlVertex3f(p01.x, p01.y, p01.z);
			}
		}
		rlEnd();
	}
	CameraHelperCommand BuildCameraHelperCommand(const CameraHelperParams& p, float helper_size)
	{
		const raylib::Vector3 target = p.target;
		const raylib::Vector3 position = p.position;
		const raylib::Vector3 cam_up = p.up;
		const float fovy = p.fovy;
		//forward
		raylib::Vector3 forward = target.Subtract(position).Normalize();
		//right up
		raylib::Vector3 right = forward.CrossProduct(cam_up).Normalize();
		raylib::Vector3 up = right.CrossProduct(forward);
		float fov = fovy * DEG2RAD;
		auto cal_plane = [position,forward,right,fov,up,&p](float size) -> std::array<raylib::Vector3, 4> {
			std::array<raylib::Vector3, 4> pl;
			raylib::Vector3 center = position.Add(forward.Scale(size));
			if (p.projection == ::CAMERA_ORTHOGRAPHIC) {
				const float aspect = (float)::GetScreenWidth() / (float)::GetScreenHeight();
				float half_height = p.fovy / 2.0f * 0.5f;
				float half_width = half_height * aspect;
				
				//up* halft_height 
				raylib::Vector3 tl = center
					.Add(up.Scale(half_height))
					.Subtract(right.Scale(half_width));
				pl[0] = tl;
				raylib::Vector3 tr = center
					.Add(up.Scale(half_height))
					.Add(right.Scale(half_width));
				pl[1] = tr;
				raylib::Vector3 bl = center
					.Subtract(up.Scale(half_height))
					.Subtract(right.Scale(half_width));
				pl[2] = bl;
				raylib::Vector3 br = center
					.Subtract(up.Scale(half_height))
					.Add(right.Scale(half_width));
				pl[3] = br;
			}
			else if (p.projection == ::CAMERA_PERSPECTIVE) {
				float h = tanf(fov * 0.5f) * size;
				float w = h;
				raylib::Vector3 tl = center
					.Add(up.Scale(h))
					.Subtract(right.Scale(w));
				pl[0] = tl;
				raylib::Vector3 tr = center
					.Add(up.Scale(h))
					.Add(right.Scale(w));
				pl[1] = tr;
				raylib::Vector3 bl = center
					.Subtract(up.Scale(h))
					.Subtract(right.Scale(w));
				pl[2] = bl;
				raylib::Vector3 br = center
					.Subtract(up.Scale(h))
					.Add(right.Scale(w));
				pl[3] = br;
			}
			return pl; 
		}; 

		auto near_pl = cal_plane(p.near);
		auto far_pl = cal_plane(p.far);
		raylib::Vector3 up_p = position.Add(up.Scale(helper_size));
		return CameraHelperCommand{ position,
			near_pl[0],
			near_pl[1],
			near_pl[2],
			near_pl[3],
			far_pl[0],
			far_pl[1],
			far_pl[2],
			far_pl[3],
			position.Add(up.Scale(helper_size)),
			position.Add(forward.Scale(helper_size)),
			position.Add(right.Scale(helper_size))
		};
	}
} // namespace Long