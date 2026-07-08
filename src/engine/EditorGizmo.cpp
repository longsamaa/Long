#include "engine/EditorGizmo.hpp"
#include "helpers/MathHelper.hpp"
#include "helpers/draw_debug_helper.hpp"
#include "core/Components.hpp"
#include "core/math/transform.hpp" // DecomposeToTransform, LocalMatrix
#include "engine/render/RenderState.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <limits>

namespace Long {
	// --- tunables (fractions of the gizmo radius) ---
	static constexpr float AXIS_LEN = 1.0f;   // arrow length
	static constexpr float AXIS_PICK_R = 0.10f;  // arrow pick half-thickness
	static constexpr float PLANE_OFFSET = 0.35f;  // plane handle offset from center
	static constexpr float PLANE_SIZE = 0.25f;  // plane handle size

	// Component-wise absolute value. Used to turn a (possibly rotated) axis vector
	// into a positive half-extent for an axis-aligned BoundingBox.
	static raylib::Vector3 AbsVec(const raylib::Vector3& v) {
		return raylib::Vector3{ std::fabsf(v.x), std::fabsf(v.y), std::fabsf(v.z) };
	}

	float EditorGizmo::GizmoScale(const BaseCamera& camera, raylib::Vector3 pos) const {
		// Constant on-screen size: scale by distance to the camera.
		return m_viewSize * pos.Distance(camera.Raw().GetPosition());
	}

	void EditorGizmo::SetGizmoToLocal()
	{
		m_local = !m_local;
	}

	raylib::Quaternion EditorGizmo::HandleOrientation(const Transform& target) const {
		if (!m_local) {
			return raylib::Quaternion{ 0, 0, 0, 1 }; // world-aligned
		}
		// While dragging a rotate ring, keep the orientation latched at drag start so
		// the ring planes stay put as the target spins under the cursor.
		if (m_dragging >= Handle::RingX && m_dragging <= Handle::RingZ) {
			return m_rotStart;
		}
		return target.quaternion;
	}

	raylib::Vector3 EditorGizmo::Axis(const Transform& target, int i) const {
		if (!m_local) return ax[i];
		return ax[i].RotateByQuaternion(HandleOrientation(target));
	}

	raylib::Vector3 EditorGizmo::Axis2(const Transform& target, int i) const {
		if (!m_local) return ax2[i];
		// ax2 is a 2-axis combo used to size/offset plane handles; rotating it as a
		// vector keeps the plane handles attached to the same two local axes.

		return raylib::Vector3(Vector3RotateByQuaternion(ax2[i], HandleOrientation(target)));
	}

	bool EditorGizmo::Update(const BaseCamera& camera, Scene& scene, entt::entity e) {
		auto& reg = scene.Registry();
		raylib::Matrix parentWorld = raylib::Matrix::Identity();
		if (const Hierarchy* h = reg.try_get<Hierarchy>(e)) {
			if (h->parent != entt::null) {
				if (const MatrixTransform* pm = reg.try_get<MatrixTransform>(h->parent)) {
					parentWorld = pm->world_matrix;
				}
			}
		}
		Transform target = DecomposeToTransform(reg.get<MatrixTransform>(e).world_matrix);

		raylib::Vector3 center = target.position;
		float r = GizmoScale(camera, center);
		raylib::Ray ray = camera.Raw().GetScreenToWorldRay(raylib::Mouse::GetPosition());
		if (!raylib::Mouse::IsButtonDown(MOUSE_BUTTON_LEFT)) {
			m_dragging = Handle::None;
		}

		if (m_dragging != Handle::None) {
			if (m_dragging >= Handle::RingX && m_dragging <= Handle::RingZ) {
				// Rotate around the ring's axis by the change in cursor angle.
				int i = (int)m_dragging - (int)Handle::RingX;
				raylib::Vector3 n = Axis(target, i);
				raylib::Vector3 u, v;
				PlaneBasis(n, u, v);
				bool ok;
				raylib::Vector3 hit = RayPlane(ray, center, n, ok);
				if (ok) {
					float angle = RingAngle(hit, center, u, v);
					float delta = angle - m_rotStartAngle;
					m_rotDelta = delta;             // remember for the HUD text
					Quaternion dq = QuaternionFromAxisAngle(n, delta);
					target.quaternion = raylib::Quaternion(QuaternionMultiply(dq, m_rotStart));
				}
			}
			else if (m_dragging >= Handle::AxisX && m_dragging <= Handle::AxisZ) {
				int i = (int)m_dragging - (int)Handle::AxisX;
				raylib::Vector3 axis = Axis(target, i);
				float t = ClosestAxisParam(ray, center, axis);
				if (debug) {
					m_closetPoint = center.Add(axis.Scale(t));
				}
				if (m_mode == Mode::Scale) {
					float factor = (fabsf(m_scaleStartParam) > 1e-4f)
						? (t / m_scaleStartParam) : 1.0f;
					raylib::Vector3 s = target.scale;
					if (i == 0) s.x = m_scaleStart.x * factor;
					else if (i == 1) s.y = m_scaleStart.y * factor;
					else s.z = m_scaleStart.z * factor;
					target.scale = s;
				}
				else {
					raylib::Vector3 hit = center.Add(axis.Scale(t));
					raylib::Vector3 delta = hit.Subtract(m_dragStartHit);
					delta = axis.Scale(delta.DotProduct(axis)); // keep only along axis
					target.position = target.position.Add(delta);
					m_dragStartHit = m_dragStartHit.Add(delta);
				}
			}
			else {
				raylib::Vector3 n = (m_dragging == Handle::PlaneXY) ? Axis(target, 2)
					: (m_dragging == Handle::PlaneXZ) ? Axis(target, 1) : Axis(target, 0);
				bool ok;
				raylib::Vector3 hit = RayPlane(ray, center, n, ok);
				if (ok) {
					raylib::Vector3 delta = hit.Subtract(m_dragStartHit);
					target.position = target.position.Add(delta);
					m_dragStartHit = hit;
				}
			}

			// `target` is the edited WORLD transform. Convert to LOCAL and store it;
			// TransformSystem will rebuild world = local * parent next frame.
			//   localMatrix = worldMatrix * inverse(parentWorld)   (column-major)
			raylib::Matrix localM = LocalMatrix(target) * MatrixInvert(parentWorld);
			Transform localT = DecomposeToTransform(localM);
			reg.patch<Transform>(e, [&](Transform& t) {
				t.position = localT.position;
				t.quaternion = localT.quaternion;
				t.scale = localT.scale;
				}); // on_update -> auto DirtyTransform
			return true;
		}

		m_hot = Handle::None;
		float bestDist = std::numeric_limits<float>::max();
		if (m_mode != Mode::Rotate) {
			const float axisLen = (1.0f + cylinder_length) * r * AXIS_LEN;
			const float pickR = r * AXIS_PICK_R; // pick radius around the arrow
			for (int i = 0; i < 3; ++i) {
				float t;
				float dist = RayAxisDistance(ray, center, Axis(target, i), axisLen, t);
				if (dist < pickR) {
					// Tie-break by camera distance to the closest point on the axis.
					float camDist = center.Add(Axis(target, i).Scale(t)).Distance(ray.position);
					if (camDist < bestDist) {
						bestDist = camDist;
						m_hot = def_handle_axis[i];
					}
				}
			}
		}

		if (m_mode == Mode::Rotate) {
			const Handle ringHandles[3] = { Handle::RingX, Handle::RingY, Handle::RingZ };
			const float ringTol = r * 0.12f; // pick band thickness
			for (int i = 0; i < 3; ++i) {
				raylib::Vector3 n = Axis(target, i);
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

		if (m_mode == Mode::Translate) {
			const int planeAxes[3][2] = { {1, 2}, {0, 2}, {0, 1} };
			const float off = PLANE_OFFSET * r;
			const float hs = PLANE_SIZE * r * 0.5f;
			for (int i = 0; i < 3; ++i) {
				raylib::Vector3 a = Axis(target, planeAxes[i][0]);
				raylib::Vector3 b = Axis(target, planeAxes[i][1]);
				raylib::Vector3 cpos = center.Add(Axis2(target, i).Scale(off));
				raylib::Vector3 n = a.CrossProduct(b); // plane normal
				bool ok;
				raylib::Vector3 hit = RayPlane(ray, cpos, n, ok);
				if (!ok) continue;
				raylib::Vector3 d = hit.Subtract(cpos);
				if (std::fabsf(d.DotProduct(a)) <= hs && std::fabsf(d.DotProduct(b)) <= hs) {
					float camDist = raylib::Vector3(ray.position).Distance(hit);
					if (camDist < bestDist) {
						bestDist = camDist;
						m_hot = def_handle_planes[i];
					}
				}
			}
		}
		if (m_hot != Handle::None && raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
			m_dragging = m_hot;
			if (m_dragging >= Handle::RingX && m_dragging <= Handle::RingZ) {
				int i = (int)m_dragging - (int)Handle::RingX;
				m_rotStart = target.quaternion;  // orientation at drag start
				raylib::Vector3 n = Axis(target, i);
				raylib::Vector3 u, v;
				PlaneBasis(n, u, v);
				bool ok;
				raylib::Vector3 hit = RayPlane(ray, center, n, ok);
				m_rotStartAngle = RingAngle(hit, center, u, v);
			}
			else if (m_dragging >= Handle::AxisX && m_dragging <= Handle::AxisZ) {
				int i = (int)m_dragging - (int)Handle::AxisX;
				raylib::Vector3 axis = Axis(target, i);
				float t = ClosestAxisParam(ray, center, axis);
				m_dragStartHit = center.Add(axis.Scale(t));
				m_scaleStartParam = t;          // for scale-ratio
				m_scaleStart = target.scale;    // scale at drag start
			}
			else {
				raylib::Vector3 n = (m_dragging == Handle::PlaneXY) ? Axis(target, 2)
					: (m_dragging == Handle::PlaneXZ) ? Axis(target, 1) : Axis(target, 0);
				bool ok;
				m_dragStartHit = RayPlane(ray, center, n, ok);
			}
		}
		return m_hot != Handle::None;
	}

	void EditorGizmo::Draw(const BaseCamera& camera, const Transform& target) {
		const raylib::Vector3& center = target.position;
		float r = GizmoScale(camera, center);
		ScopedDepthTest depth(false);      // gizmo always on top
		ScopedBackfaceCull cull(false);    // draw both sides of planes/torus
		const bool scaleMode = (m_mode == Mode::Scale);
		const bool rotateMode = (m_mode == Mode::Rotate);
		if (!rotateMode) {
			for (int i = 0; i < 3; ++i) {
				Handle hAxis = def_handle_axis[i];
				raylib::Color c = (m_hot == hAxis || m_dragging == hAxis) ? raylib::Color::Yellow() : axisCol[i];
				raylib::Vector3 axis = Axis(target, i);
				raylib::Vector3 tip = center.Add(axis.Scale(r * AXIS_LEN));
				tip.DrawLine3D(center, c);
				if (scaleMode) {
					float hs = r * 0.08f;
					raylib::Vector3 half = raylib::Vector3{ hs, hs, hs };
					raylib::BoundingBox box{ tip.Subtract(half), tip.Add(half) };
					box.Draw(c);
				}
				else {
					raylib::Vector3 tipEnd = tip.Add(axis.Scale(r * cylinder_length));
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
				PlaneBasis(Axis(target, i), u, v);
				DrawTorus3D(center, u, v, r, tube, c);
			}
		}

		if (m_mode == Mode::Translate) {
			raylib::Color planeCol[3] = { raylib::Color::Red(), raylib::Color::Green(), raylib::Color::Blue() };
			// The two local axes that span each plane handle. Order matches
			// def_handle_planes = { PlaneYZ, PlaneXZ, PlaneXY }.
			const int planeAxes[3][2] = { {1, 2}, {0, 2}, {0, 1} };
			for (int i = 0; i < 3; ++i) {
				Handle h = def_handle_planes[i];
				raylib::Color c = (m_hot == h || m_dragging == h) ? raylib::Color::Yellow() : planeCol[i];
				c.a = 140;
				raylib::Vector3 a = Axis(target, planeAxes[i][0]);
				raylib::Vector3 b = Axis(target, planeAxes[i][1]);
				float off = PLANE_OFFSET * r;
				float hs = PLANE_SIZE * r * 0.5f;
				raylib::Vector3 cpos = center.Add(Axis2(target, i).Scale(off));
				raylib::Vector3 ea = a.Scale(hs);
				raylib::Vector3 eb = b.Scale(hs);
				raylib::Vector3 p0 = cpos.Subtract(ea).Subtract(eb);
				raylib::Vector3 p1 = cpos.Add(ea).Subtract(eb);
				raylib::Vector3 p2 = cpos.Add(ea).Add(eb);
				raylib::Vector3 p3 = cpos.Subtract(ea).Add(eb);
				::DrawTriangle3D(p0, p1, p2, c);
				::DrawTriangle3D(p0, p2, p3, c);
				::DrawTriangle3D(p0, p2, p1, c);
				::DrawTriangle3D(p0, p3, p2, c);
				raylib::Color edge = c; edge.a = 255;
				p0.DrawLine3D(p1, edge); p1.DrawLine3D(p2, edge);
				p2.DrawLine3D(p3, edge); p3.DrawLine3D(p0, edge);
			}
		}
		Handle active = (m_dragging != Handle::None) ? m_dragging : m_hot;
		if (active >= Handle::AxisX && active <= Handle::AxisZ) {
			int i = (int)active - (int)Handle::AxisX;
			raylib::Vector3 farHalf = Axis(target, i).Scale(10000.0f);
			center.Add(farHalf).DrawLine3D(center.Subtract(farHalf), raylib::Color::White());
		}

		if (debug) {
			drawDebugGizmo(target, r);
		}
		// depth test + backface culling restored by scope guards
	}

	void EditorGizmo::DrawScreenGuide(const BaseCamera& camera, const Transform& target) {
		if ((m_dragging >= Handle::PlaneXY && m_dragging <= Handle::PlaneYZ) || m_dragging == Handle::None) {
			return;
		}
		raylib::Vector2 centerScreen = camera.Raw().GetWorldToScreen(target.position);
		raylib::Vector2 mouse = raylib::Mouse::GetPosition();
		::DrawLineEx(centerScreen, mouse, 1.5f, raylib::Color::White());
	}

	void EditorGizmo::drawDebugGizmo(const Transform& target, const float& scale) {
		const raylib::Vector3& center = target.position;
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