#include "CameraParameterInspector.hpp"
#include "core/Components.hpp"
#include "imgui.h"
#include "raylib.h" // CAMERA_PERSPECTIVE / CAMERA_ORTHOGRAPHIC

namespace Long {

	bool CameraParameterInspector::Draw(GameCameraParameter& param) {
		bool changed = false;
		{
			const char* items[] = { "Perspective", "Orthographic" };
			// projection stores CAMERA_PERSPECTIVE(0) / CAMERA_ORTHOGRAPHIC(1).
			int current = (param.projection == ::CAMERA_ORTHOGRAPHIC) ? 1 : 0;
			if (ImGui::Combo("Projection", &current, items, IM_ARRAYSIZE(items))) {
				uint32_t next = (current == 1) ? (uint32_t)::CAMERA_ORTHOGRAPHIC
											   : (uint32_t)::CAMERA_PERSPECTIVE;
				if (next != param.projection) {
					param.projection = next;
					changed = true;
				}
			}
		}

		const char* fovLabel = (param.projection == ::CAMERA_ORTHOGRAPHIC)
								   ? "Size" : "FOV";
		if (ImGui::DragFloat(fovLabel, &param.fov, 0.1f, 1.0f, 179.0f, "%.1f")) {
			changed = true;
		}

		if (ImGui::DragFloat("Near", &param.near, 0.01f, 0.001f, param.far, "%.3f")) {
			if (param.near < 0.001f) param.near = 0.001f;
			if (param.near >= param.far) param.near = param.far - 0.001f;
			changed = true;
		}
		if (ImGui::DragFloat("Far", &param.far, 0.5f, param.near + 0.001f, 100000.0f, "%.2f")) {
			if (param.far <= param.near) param.far = param.near + 0.001f;
			changed = true;
		}

		if (changed) {
			++param.version;
		}
		return changed;
	}

}
