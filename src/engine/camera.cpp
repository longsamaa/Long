#include "engine/camera.hpp"

namespace Long {
	EditorCamera::EditorCamera() {
		m_camera.up = { 0.0f, 1.0f, 0.0f };
		m_camera.fovy = 45.0f;
		m_camera.projection = ::CAMERA_PERSPECTIVE;
		UpdateCameraVectors();
	}

	void EditorCamera::UpdateCameraVectors() {
		float yawRad = m_yaw * DEG2RAD;
		float pitchRad = m_pitch * DEG2RAD;
		raylib::Vector3 offset{
			::cosf(pitchRad) * ::cosf(yawRad),
			::sinf(pitchRad),
			::cosf(pitchRad) * ::sinf(yawRad)
		};
		m_camera.target = m_target;
		m_camera.position = ::Vector3Add(m_target, ::Vector3Scale(offset, m_distance));
	}

	void EditorCamera::Update(float dt) {
		(void)dt;
		raylib::Vector2 delta = ::GetMouseDelta();
		if (::IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
			m_yaw += delta.x * m_orbitSpeed;
			m_pitch += delta.y * m_orbitSpeed;
			if (m_pitch > m_maxPitch) m_pitch = m_maxPitch;
			if (m_pitch < m_minPitch) m_pitch = m_minPitch;
		}
		if (::IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
			Vector3 forward = ::Vector3Normalize(::Vector3Subtract(m_camera.target, m_camera.position));
			Vector3 right = ::Vector3Normalize(::Vector3CrossProduct(forward, m_camera.up));
			Vector3 up = ::Vector3CrossProduct(right, forward);
			float scale = m_panSpeed * m_distance;
			Vector3 move = ::Vector3Add(
				::Vector3Scale(right, -delta.x * scale),
				::Vector3Scale(up, delta.y * scale));
			m_target = ::Vector3Add(m_target, move);
		}
		UpdateCameraVectors();
	}

	void EditorCamera::FocusOn(const raylib::Vector3& point) {
		m_target = point;
		UpdateCameraVectors();
	}

	void EditorCamera::ZoomToward(float wheel, const raylib::Vector3& pivot) {
		if (wheel == 0.0f) {
			return;
		}
		float oldDistance = m_distance;
		m_distance -= wheel * m_zoomSpeed;
		if (m_distance < m_minDistance) m_distance = m_minDistance;
		if (m_distance > m_maxDistance) m_distance = m_maxDistance;
		float lerp = 1.0f - (m_distance / oldDistance);
		m_target = ::Vector3Add(m_target,
			::Vector3Scale(::Vector3Subtract(pivot, m_target), lerp));
		UpdateCameraVectors();
	}
} // namespace Long