#pragma once
#ifndef _EDITOR_GIZMO_HPP_
#define _EDITOR_GIZMO_HPP_

#include "raylib-cpp.hpp"
#include "core/Components.hpp"

namespace Long {

	// Our own transform gizmo (no external lib). CPU ray-collision picking, so it
	// doesn't touch framebuffer/GL state. Starts with TRANSLATE (3 axis arrows +
	// 3 plane handles); rotate/scale added later.
	class EditorGizmo {
	public:
		// Which part of the gizmo the cursor is on / dragging.
		enum class Handle {
			None,
			AxisX, AxisY, AxisZ,        // move along one axis
			PlaneXY, PlaneXZ, PlaneYZ,  // move on a plane
		};

		// Process input for the gizmo at `target`'s position; edits target.
		// Returns true if the gizmo consumed the mouse (hovered or dragging).
		bool Update(const raylib::Camera3D& camera, Transform& target);

		// Draw the gizmo handles. Call inside the camera's 3D mode.
		void Draw(const raylib::Camera3D& camera, const Transform& target);

		// Draw the 2D overlay (line from the gizmo center to the cursor). Call
		// AFTER EndMode3D (it's screen-space).
		void DrawScreenGuide(const raylib::Camera3D& camera, const Transform& target);

		bool IsActive() const { return m_dragging != Handle::None; }
		bool IsHot() const { return m_hot != Handle::None; }

	private:
		// On-screen size as a fraction of camera distance (constant screen size).
		float GizmoScale(const raylib::Camera3D& camera, raylib::Vector3 pos) const;

		Handle m_hot = Handle::None;      // hovered this frame
		Handle m_dragging = Handle::None; // axis being dragged
		raylib::Vector3 m_dragStartHit{}; // world point where the drag began

		float m_viewSize = 0.12f;         // gizmo size (fraction of distance)
	};

} // namespace Long

#endif // !_EDITOR_GIZMO_HPP_
