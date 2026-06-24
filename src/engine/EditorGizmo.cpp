#include "engine/EditorGizmo.hpp"
#include "helpers/MathHelper.hpp"
#include "helpers/draw_debug_helper.hpp"
#include "core/Components.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <limits>

namespace Long {
	// --- tunables (fractions of the gizmo radius) ---
	static constexpr float AXIS_LEN = 1.0f;   // arrow length
	static constexpr float AXIS_PICK_R = 0.10f;  // arrow pick half-thickness
	static constexpr float PLANE_OFFSET = 0.35f;  // plane handle offset from center
	static constexpr float PLANE_SIZE = 0.25f;  // plane handle size

	float EditorGizmo::GizmoScale(const raylib::Camera3D& camera, raylib::Vector3 pos) const {
		// Constant on-screen size: scale by distance to the camera.
		return m_viewSize * pos.Distance(camera.position);
	}

	bool EditorGizmo::Update(const raylib::Camera3D& camera, Transform& target) {
		raylib::Vector3 center = target.getPos();
		float r = GizmoScale(camera, center);
		raylib::Ray ray = camera.GetScreenToWorldRay(raylib::Mouse::GetPosition());

		// --- Release ---
		if (!raylib::Mouse::IsButtonDown(MOUSE_BUTTON_LEFT)) {
			m_dragging = Handle::None;
		}

		// --- While dragging: apply movement ---
		if (m_dragging != Handle::None) {
			if (m_dragging >= Handle::RingX && m_dragging <= Handle::RingZ) {
				// Rotate around the ring's axis by the change in cursor angle.
				int i = (int)m_dragging - (int)Handle::RingX;
				raylib::Vector3 n = ax[i];
				raylib::Vector3 u, v;
				PlaneBasis(n, u, v);
				bool ok;
				raylib::Vector3 hit = RayPlane(ray, center, n, ok);
				if (ok) {
					float angle = RingAngle(hit, center, u, v);
					float delta = angle - m_rotStartAngle;
					m_rotDelta = delta;             // remember for the HUD text
					Quaternion dq = QuaternionFromAxisAngle(n, delta);
					// Apply the delta in world space on top of the start orientation.
					target.setQuaternion(raylib::Quaternion(QuaternionMultiply(dq, m_rotStart)));
				}
				return true;
			}
			if (m_dragging >= Handle::AxisX && m_dragging <= Handle::AxisZ) {
				int i = (int)m_dragging - (int)Handle::AxisX;
				float t = ClosestAxisParam(ray, center, ax[i]);
				if (debug) {
					m_closetPoint = center.Add(ax[i].Scale(t));
				}
				if (m_mode == Mode::Scale) {
					// Scale along the axis by how far the cursor moved relative to
					// where the drag began (ratio keeps it frame-rate independent).
					float factor = (fabsf(m_scaleStartParam) > 1e-4f)
						? (t / m_scaleStartParam) : 1.0f;
					raylib::Vector3 s = target.getScale();
					if (i == 0) s.x = m_scaleStart.x * factor;
					else if (i == 1) s.y = m_scaleStart.y * factor;
					else s.z = m_scaleStart.z * factor;
					target.setScale(s);
				}
				else {
					raylib::Vector3 hit = center.Add(ax[i].Scale(t));
					raylib::Vector3 delta = hit.Subtract(m_dragStartHit);
					delta = ax[i].Scale(delta.DotProduct(ax[i])); // keep only along axis
					target.setPos(target.getPos().Add(delta)); 
					//target = raylib::Vector3(target.position).Add(delta);
					m_dragStartHit = m_dragStartHit.Add(delta);
				}
			}
			else {
				// Plane normal = the axis NOT in the plane (ax[] index).
				int planeIdx = (int)m_dragging - (int)Handle::PlaneXY; // 0,1,2
				raylib::Vector3 n = (m_dragging == Handle::PlaneXY) ? ax[2]
					: (m_dragging == Handle::PlaneXZ) ? ax[1] : ax[0];
				bool ok;
				raylib::Vector3 hit = RayPlane(ray, center, n, ok);
				if (ok) {
					raylib::Vector3 delta = hit.Subtract(m_dragStartHit);
					//target.position = raylib::Vector3(target.position).Add(delta);
					target.setPos(target.getPos().Add(delta)); 
					m_dragStartHit = hit;
				}
				(void)planeIdx;
			}
			return true;
		}

		// --- Not dragging: hover-test the handles, pick the closest ---
		m_hot = Handle::None;
		float bestDist = std::numeric_limits<float>::max();
		// Axis arrows (translate/scale): an AABB enclosing center->tip.
		if (m_mode != Mode::Rotate) {
			for (int i = 0; i < 3; ++i) {
				raylib::Vector3 tip = center.Add(ax[i].Scale((1.0f + cylinder_length) * r * AXIS_LEN));
				raylib::Vector3 pad = ax2[i].Scale(r * AXIS_PICK_R);
				raylib::BoundingBox box{ center.Min(tip).Subtract(pad), center.Max(tip).Add(pad) };
				raylib::RayCollision col = ray.GetCollision(box);
				if (col.hit && col.distance < bestDist) {
					bestDist = col.distance;
					m_hot = def_handle_axis[i];
				}
			}
		}

		// Rotate rings: intersect the ray with each ring's plane, then check the
		// hit lies near the ring radius (within a tolerance band).
		if (m_mode == Mode::Rotate) {
			const Handle ringHandles[3] = { Handle::RingX, Handle::RingY, Handle::RingZ };
			const float ringTol = r * 0.12f; // pick band thickness
			for (int i = 0; i < 3; ++i) {
				raylib::Vector3 n = ax[i];
				bool ok;
				raylib::Vector3 hit = RayPlane(ray, center, n, ok);
				if (!ok) continue;
				float dist = hit.Distance(center);
				if (std::fabsf(dist - r) < ringTol) {
					float camDist = raylib::Vector3(ray.position).Distance(hit);
					if (camDist < bestDist) {
						bestDist = camDist;
						m_hot = ringHandles[i];
					}
				}
			}
		}

		// Plane handles: small boxes offset along the plane's two axes (ax2).
		// Only in translate mode.
		if (m_mode == Mode::Translate) {
			for (int i = 0; i < 3; ++i) {
				raylib::Vector3 tip = center.Add(ax2[i].Scale(PLANE_OFFSET * r));
				raylib::Vector3 half = ax2[i].Scale(PLANE_SIZE * r * 0.5f);
				raylib::BoundingBox box{ tip.Subtract(half), tip.Add(half) };
				raylib::RayCollision col = ray.GetCollision(box);
				if (col.hit && col.distance < bestDist) {
					bestDist = col.distance;
					m_hot = def_handle_planes[i];
				}
			}
		}
		if (m_hot != Handle::None && raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
			m_dragging = m_hot;
			if (m_dragging >= Handle::RingX && m_dragging <= Handle::RingZ) {
				int i = (int)m_dragging - (int)Handle::RingX;
				raylib::Vector3 n = ax[i];
				raylib::Vector3 u, v;
				PlaneBasis(n, u, v);
				bool ok;
				raylib::Vector3 hit = RayPlane(ray, center, n, ok);
				m_rotStartAngle = RingAngle(hit, center, u, v);
				m_rotStart = target.getQuaternion();  // orientation at drag start
			}
			else if (m_dragging >= Handle::AxisX && m_dragging <= Handle::AxisZ) {
				int i = (int)m_dragging - (int)Handle::AxisX;
				float t = ClosestAxisParam(ray, center, ax[i]);
				m_dragStartHit = center.Add(ax[i].Scale(t));
				m_scaleStartParam = t;          // for scale-ratio
				m_scaleStart = target.getScale();    // scale at drag start
			}
			else {
				raylib::Vector3 n = (m_dragging == Handle::PlaneXY) ? ax[2]
					: (m_dragging == Handle::PlaneXZ) ? ax[1] : ax[0];
				bool ok;
				m_dragStartHit = RayPlane(ray, center, n, ok);
			}
		}

		return m_hot != Handle::None;
	}

	void EditorGizmo::Draw(const raylib::Camera3D& camera, const Transform& target) {
		const raylib::Vector3& center = target.getPos();
		float r = GizmoScale(camera, center);
		rlDisableDepthTest(); // gizmo always on top
		const bool scaleMode = (m_mode == Mode::Scale);
		const bool rotateMode = (m_mode == Mode::Rotate);
		if (!rotateMode) {
			for (int i = 0; i < 3; ++i) {
				Handle hAxis = def_handle_axis[i];
				raylib::Color c = (m_hot == hAxis || m_dragging == hAxis) ? raylib::Color::Yellow() : axisCol[i];
				raylib::Vector3 tip = center.Add(ax[i].Scale(r * AXIS_LEN));
				tip.DrawLine3D(center, c);
				if (scaleMode) {
					float hs = r * 0.08f;
					raylib::Vector3 half = raylib::Vector3{ hs, hs, hs };
					raylib::BoundingBox box{ tip.Subtract(half), tip.Add(half) };
					box.Draw(c);
				}
				else {
					raylib::Vector3 tipEnd = tip.Add(ax[i].Scale(r * cylinder_length));
					::DrawCylinderEx(tip, tipEnd, r * 0.06f, 0.0f, 12, c);
				}
			}
		}

		if (rotateMode) {
			const Handle ringHandles[3] = { Handle::RingX, Handle::RingY, Handle::RingZ };
			for (int i = 0; i < 3; ++i) {
				bool activeRing = (m_hot == ringHandles[i] || m_dragging == ringHandles[i]);
				raylib::Color c = activeRing ? raylib::Color::Yellow() : axisCol[i];
				float tube = (activeRing ? 0.05f : 0.03f) * r;
				raylib::Vector3 u, v;
				PlaneBasis(ax[i], u, v);
				DrawTorus3D(center, u, v, r, tube, c);
			}
		}

		if (m_mode == Mode::Translate) {
			raylib::Color planeCol[3] = { raylib::Color::Red(), raylib::Color::Green(), raylib::Color::Blue() };
			rlDisableBackfaceCulling();
			for (int i = 0; i < 3; ++i) {
				Handle h = def_handle_planes[i];
				raylib::Color c = (m_hot == h || m_dragging == h) ? raylib::Color::Yellow() : planeCol[i];
				c.a = 140;
				raylib::Vector3 cpos = center.Add(ax2[i].Scale(PLANE_OFFSET * r));
				float hs = PLANE_SIZE * r * 0.5f;
				raylib::Vector3 half = ax2[i].Scale(hs);
				raylib::BoundingBox box{ cpos.Subtract(half), cpos.Add(half) };
				box.Draw(c);
			}
		}
		Handle active = (m_dragging != Handle::None) ? m_dragging : m_hot;
		if (active >= Handle::AxisX && active <= Handle::AxisZ) {
			int i = (int)active - (int)Handle::AxisX;
			raylib::Vector3 farHalf = ax[i].Scale(10000.0f);
			center.Add(farHalf).DrawLine3D(center.Subtract(farHalf), raylib::Color::White());
		}

		if (debug) {
			drawDebugGizmo(target, r);
		}
		rlEnableDepthTest();
	}

	void EditorGizmo::DrawScreenGuide(const raylib::Camera3D& camera, const Transform& target) {
		if ((m_dragging >= Handle::PlaneXY && m_dragging <= Handle::PlaneYZ) || m_dragging == Handle::None) {
			return;
		}
		raylib::Vector2 centerScreen = camera.GetWorldToScreen(target.getPos());
		raylib::Vector2 mouse = raylib::Mouse::GetPosition();
		::DrawLineEx(centerScreen, mouse, 1.5f, raylib::Color::White());
	}

	void EditorGizmo::drawDebugGizmo(const Transform& target, const float& scale) {
		const raylib::Vector3& center = target.getPos();
		center.DrawSphere(0.1f, raylib::Color::Red());
		for (int i = 0; i < 3; ++i) {
			raylib::Vector3 tip = center.Add(ax[i].Scale((1.0f + cylinder_length) * AXIS_LEN * scale));
			raylib::Vector3 pad = ax2[i].Scale(AXIS_PICK_R * scale);
			raylib::BoundingBox box{ center.Min(tip).Subtract(pad), center.Max(tip).Add(pad) };
			tip.DrawLine3D(center, raylib::Color::Red());
			box.Draw(raylib::Color::Gray());
		}
		for (int i = 0; i < 3; ++i) {
			raylib::Vector3 tip = center.Add(ax2[i].Scale(PLANE_OFFSET * scale));
			raylib::Vector3 half = ax2[i].Scale(PLANE_SIZE * scale * 0.5f);
			raylib::BoundingBox box{ tip.Subtract(half), tip.Add(half) };
			tip.DrawSphere(0.1f, raylib::Color::Red());
			box.Draw(raylib::Color::Gray());
		}
		m_closetPoint.DrawSphere(0.1f, raylib::Color::Green());
	}
} // namespace Long