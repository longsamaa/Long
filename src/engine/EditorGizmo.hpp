#pragma once
#ifndef _EDITOR_GIZMO_HPP_
#define _EDITOR_GIZMO_HPP_

#include "raylib-cpp.hpp"
#include "core/Components.hpp"
#include <vector>
namespace Long {
	class EditorGizmo {
	public:
		enum class Handle {
			None,
			AxisX, AxisY, AxisZ,        // move/scale along one axis
			PlaneXY, PlaneXZ, PlaneYZ,  // move on a plane
			RingX, RingY, RingZ,        // rotate around one axis
		};

		// What the gizmo edits. (Rotate/Scale TODO; only Translate is wired up.)
		enum class Mode { Translate, Rotate, Scale };
		Mode GetMode() const { return m_mode; }
		void SetMode(Mode mode) { m_mode = mode; }
		// Toggle the gizmo between local space (handles follow the target's
		// orientation) and world space (handles stay axis-aligned).
		void SetGizmoToLocal();
		bool IsLocal() const { return m_local; }
		bool Update(const raylib::Camera3D& camera, Transform& target);
		void Draw(const raylib::Camera3D& camera, const Transform& target);
		void DrawScreenGuide(const raylib::Camera3D& camera, const Transform& target);
		bool IsActive() const { return m_dragging != Handle::None; }
		bool IsHot() const { return m_hot != Handle::None; }
		// True while dragging a rotate ring; `outDeg` gets the angle delta (degrees).
		bool IsRotating(float& outDeg) const {
			if (m_dragging >= Handle::RingX && m_dragging <= Handle::RingZ) {
				outDeg = m_rotDelta * RAD2DEG;
				return true;
			}
			return false;
		}
		void drawDebugGizmo(const Transform& target, const float& scale);
	private:
		float GizmoScale(const raylib::Camera3D& camera, raylib::Vector3 pos) const;
		// Orientation the handles are drawn/picked with: the target's rotation when
		// in local mode (and during a rotate drag, the orientation latched at drag
		// start so the ring planes don't drift), identity in world mode.
		raylib::Quaternion HandleOrientation(const Transform& target) const;
		// ax[i] / ax2[i] rotated into the current handle space (see HandleOrientation).
		raylib::Vector3 Axis(const Transform& target, int i) const;
		raylib::Vector3 Axis2(const Transform& target, int i) const;
		Handle m_hot = Handle::None;      // hovered this frame
		Handle m_dragging = Handle::None; // axis being dragged
		raylib::Vector3 m_dragStartHit{}; // world point where the drag began
		raylib::Vector3 m_closetPoint{ 0.0,0.0,0.0 }; // For debug
		// Scale drag state: param along the axis + scale when the drag began.
		float m_scaleStartParam = 0.0f;
		raylib::Vector3 m_scaleStart{ 1.0f, 1.0f, 1.0f };
		// Rotate drag state: cursor angle on the ring plane when the drag began.
		float m_rotStartAngle = 0.0f;
		raylib::Quaternion m_rotStart{ 0.0f, 0.0f, 0.0f, 1.0f };
		float m_rotDelta = 0.0f;          // current rotation delta (radians), for the HUD
		float cylinder_length{ 0.2f }; 
		const raylib::Vector3 ax[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
		const raylib::Vector3 ax2[3] = { {0,1,1}, {1,0,1}, {1,1,0} };
		const std::vector<Handle> def_handle_planes{ Handle::PlaneYZ , Handle::PlaneXZ, Handle::PlaneXY }; 
		const std::vector<Handle> def_handle_axis{ Handle::AxisX , Handle::AxisY, Handle::AxisZ }; 
		raylib::Color axisCol[3] = { raylib::Color::Red(), raylib::Color::Green(), raylib::Color::Blue() };
		bool debug{ false };
		bool m_local{ false };            // handles follow the target's orientation
		float m_viewSize = 0.12f;         // gizmo size (fraction of distance)
		Mode m_mode = Mode::Translate;    // current edit mode
	};

} // namespace Long

#endif // !_EDITOR_GIZMO_HPP_
