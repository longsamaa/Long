#include "engine/EditorGizmo.hpp"
#include "raymath.h"
#include "rlgl.h"

namespace Long {

	// --- tunables (fractions of the gizmo radius) ---
	static constexpr float AXIS_LEN      = 1.0f;   // arrow length
	static constexpr float AXIS_PICK_R   = 0.10f;  // arrow pick radius
	static constexpr float PLANE_OFFSET  = 0.35f;  // plane handle offset from center
	static constexpr float PLANE_SIZE    = 0.25f;  // plane handle size

	float EditorGizmo::GizmoScale(const raylib::Camera3D& camera, raylib::Vector3 pos) const {
		return m_viewSize * Vector3Distance(camera.position, pos);
	}

	// Closest point on the ray to an infinite axis line through `origin` along
	// `dir`; returns the parameter `t` along the axis. (Used for axis dragging.)
	static float ClosestAxisParam(Ray ray, Vector3 origin, Vector3 dir) {
		// Solve for the point on the axis closest to the ray.
		Vector3 w0 = Vector3Subtract(ray.position, origin);
		float a = Vector3DotProduct(ray.direction, ray.direction);
		float b = Vector3DotProduct(ray.direction, dir);
		float c = Vector3DotProduct(dir, dir);
		float d = Vector3DotProduct(ray.direction, w0);
		float e = Vector3DotProduct(dir, w0);
		float denom = a * c - b * b;
		if (fabsf(denom) < 1e-6f) return 0.0f;
		// tAxis = parameter along `dir`.
		return (a * e - b * d) / denom;
	}

	// Intersect a ray with the plane through `p` with normal `n`. Returns hit
	// point; sets ok=false if parallel.
	static Vector3 RayPlane(Ray ray, Vector3 p, Vector3 n, bool* ok) {
		float denom = Vector3DotProduct(n, ray.direction);
		if (fabsf(denom) < 1e-6f) { *ok = false; return p; }
		float t = Vector3DotProduct(Vector3Subtract(p, ray.position), n) / denom;
		*ok = (t > 0.0f);
		return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
	}

	bool EditorGizmo::Update(const raylib::Camera3D& camera, Transform& target) {
		Vector3 center = target.position;
		float r = GizmoScale(camera, center);
		Ray ray = GetScreenToWorldRay(GetMousePosition(), camera);

		const Vector3 ax[3] = { {1,0,0}, {0,1,0}, {0,0,1} };

		// --- Release ---
		if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			m_dragging = Handle::None;
		}

		// --- While dragging: apply movement ---
		if (m_dragging != Handle::None) {
			switch (m_dragging) {
			case Handle::AxisX:
			case Handle::AxisY:
			case Handle::AxisZ: {
				int i = (int)m_dragging - (int)Handle::AxisX;
				// Current closest point on the axis; move target there relative
				// to where the drag started.
				float t = ClosestAxisParam(ray, center, ax[i]);
				Vector3 hit = Vector3Add(center, Vector3Scale(ax[i], t));
				Vector3 delta = Vector3Subtract(hit, m_dragStartHit);
				// Keep delta only along the axis.
				delta = Vector3Scale(ax[i], Vector3DotProduct(delta, ax[i]));
				target.position = Vector3Add(target.position, delta);
				m_dragStartHit = Vector3Add(m_dragStartHit, delta);
				break;
			}
			case Handle::PlaneXY:
			case Handle::PlaneXZ:
			case Handle::PlaneYZ: {
				// Plane normal = the axis NOT in the plane.
				Vector3 n = (m_dragging == Handle::PlaneXY) ? ax[2]
					: (m_dragging == Handle::PlaneXZ) ? ax[1] : ax[0];
				bool ok;
				Vector3 hit = RayPlane(ray, center, n, &ok);
				if (ok) {
					Vector3 delta = Vector3Subtract(hit, m_dragStartHit);
					target.position = Vector3Add(target.position, delta);
					m_dragStartHit = hit;
				}
				break;
			}
			default: break;
			}
			return true;
		}

		// --- Not dragging: hover-test the handles, pick the closest hit ---
		m_hot = Handle::None;
		float bestDist = 1e30f;

		// Axis arrows (test as cylinders along each axis).
		for (int i = 0; i < 3; ++i) {
			Vector3 tip = Vector3Add(center, Vector3Scale(ax[i], r * AXIS_LEN));
			RayCollision col = GetRayCollisionBox(ray, BoundingBox{
				Vector3Subtract(Vector3Min(center, tip), Vector3Scale({1,1,1}, r * AXIS_PICK_R)),
				Vector3Add(Vector3Max(center, tip), Vector3Scale({1,1,1}, r * AXIS_PICK_R)) });
			if (col.hit && col.distance < bestDist) {
				bestDist = col.distance;
				m_hot = (Handle)((int)Handle::AxisX + i);
			}
		}

		// Plane handles (small quads near the center).
		struct PlaneDef { Handle h; Vector3 n; Vector3 u; Vector3 v; };
		PlaneDef planes[3] = {
			{ Handle::PlaneXY, ax[2], ax[0], ax[1] },
			{ Handle::PlaneXZ, ax[1], ax[0], ax[2] },
			{ Handle::PlaneYZ, ax[0], ax[1], ax[2] },
		};
		for (auto& pl : planes) {
			Vector3 c = Vector3Add(center,
				Vector3Add(Vector3Scale(pl.u, r * PLANE_OFFSET),
						   Vector3Scale(pl.v, r * PLANE_OFFSET)));
			float hs = r * PLANE_SIZE * 0.5f;
			Vector3 a = Vector3Subtract(Vector3Subtract(c, Vector3Scale(pl.u, hs)), Vector3Scale(pl.v, hs));
			Vector3 b = Vector3Add(a, Vector3Scale(pl.u, hs * 2));
			Vector3 cc = Vector3Add(b, Vector3Scale(pl.v, hs * 2));
			Vector3 d = Vector3Add(a, Vector3Scale(pl.v, hs * 2));
			RayCollision col = GetRayCollisionQuad(ray, a, b, cc, d);
			if (col.hit && col.distance < bestDist) {
				bestDist = col.distance;
				m_hot = pl.h;
			}
		}

		// --- Begin drag on click ---
		if (m_hot != Handle::None && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			m_dragging = m_hot;
			// Record the world hit where the drag starts.
			if (m_dragging >= Handle::AxisX && m_dragging <= Handle::AxisZ) {
				int i = (int)m_dragging - (int)Handle::AxisX;
				float t = ClosestAxisParam(ray, center, ax[i]);
				m_dragStartHit = Vector3Add(center, Vector3Scale(ax[i], t));
			} else {
				Vector3 n = (m_dragging == Handle::PlaneXY) ? ax[2]
					: (m_dragging == Handle::PlaneXZ) ? ax[1] : ax[0];
				bool ok;
				m_dragStartHit = RayPlane(ray, center, n, &ok);
			}
		}

		return m_hot != Handle::None;
	}

	void EditorGizmo::Draw(const raylib::Camera3D& camera, const Transform& target) {
		Vector3 center = target.position;
		float r = GizmoScale(camera, center);
		const Vector3 ax[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
		Color axisCol[3] = { RED, GREEN, BLUE };

		rlDisableDepthTest(); // gizmo always on top

		// Axis arrows.
		for (int i = 0; i < 3; ++i) {
			Handle hAxis = (Handle)((int)Handle::AxisX + i);
			Color c = (m_hot == hAxis || m_dragging == hAxis) ? YELLOW : axisCol[i];
			Vector3 tip = Vector3Add(center, Vector3Scale(ax[i], r * AXIS_LEN));
			DrawLine3D(center, tip, c);
			Vector3 tipEnd = Vector3Add(tip, Vector3Scale(ax[i], r * 0.2f));
			DrawCylinderEx(tip, tipEnd, r * 0.06f, 0.0f, 12, c);
		}

		// Plane handles.
		struct PlaneDef { Handle h; Vector3 u; Vector3 v; Color col; };
		PlaneDef planes[3] = {
			{ Handle::PlaneXY, ax[0], ax[1], BLUE  },
			{ Handle::PlaneXZ, ax[0], ax[2], GREEN },
			{ Handle::PlaneYZ, ax[1], ax[2], RED   },
		};
		rlDisableBackfaceCulling();
		for (auto& pl : planes) {
			Color c = (m_hot == pl.h || m_dragging == pl.h) ? YELLOW : pl.col;
			c.a = 140;
			Vector3 cpos = Vector3Add(center,
				Vector3Add(Vector3Scale(pl.u, r * PLANE_OFFSET),
						   Vector3Scale(pl.v, r * PLANE_OFFSET)));
			float hs = r * PLANE_SIZE * 0.5f;
			Vector3 a = Vector3Subtract(Vector3Subtract(cpos, Vector3Scale(pl.u, hs)), Vector3Scale(pl.v, hs));
			Vector3 b = Vector3Add(a, Vector3Scale(pl.u, hs * 2));
			Vector3 cc = Vector3Add(b, Vector3Scale(pl.v, hs * 2));
			Vector3 d = Vector3Add(a, Vector3Scale(pl.v, hs * 2));
			DrawTriangle3D(a, b, cc, c);
			DrawTriangle3D(a, cc, d, c);
		}

		// Guide lines for the hovered/dragged handle (like the reference gizmo).
		Handle active = (m_dragging != Handle::None) ? m_dragging : m_hot;
		if (active >= Handle::AxisX && active <= Handle::AxisZ) {
			// Axis: one long line along that axis.
			int i = (int)active - (int)Handle::AxisX;
			Vector3 half = Vector3Scale(ax[i], 1000.0f);
			DrawLine3D(Vector3Subtract(center, half), Vector3Add(center, half), WHITE);
		}
		else if (active >= Handle::PlaneXY && active <= Handle::PlaneYZ) {
			// Plane: two long lines along the plane's two axes.
			Vector3 u = (active == Handle::PlaneXY) ? ax[0] : (active == Handle::PlaneXZ) ? ax[0] : ax[1];
			Vector3 v = (active == Handle::PlaneXY) ? ax[1] : (active == Handle::PlaneXZ) ? ax[2] : ax[2];
			Vector3 hu = Vector3Scale(u, 1000.0f);
			Vector3 hv = Vector3Scale(v, 1000.0f);
			DrawLine3D(Vector3Subtract(center, hu), Vector3Add(center, hu), WHITE);
			DrawLine3D(Vector3Subtract(center, hv), Vector3Add(center, hv), WHITE);
		}

		rlEnableDepthTest();
	}

	void EditorGizmo::DrawScreenGuide(const raylib::Camera3D& camera, const Transform& target) {
		// Only while actively dragging. Project the gizmo center to the screen and
		// draw a 2D line from it to the mouse cursor (like the reference gizmo).
		if (m_dragging == Handle::None) {
			return;
		}
		Vector2 centerScreen = GetWorldToScreen(target.position, camera);
		Vector2 mouse = GetMousePosition();
		DrawLineEx(centerScreen, mouse, 1.5f, WHITE);
	}

} // namespace Long
