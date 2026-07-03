#include "LightInspector.hpp"
#include "core/Components.hpp"
#include "imgui.h"

namespace Long {

	bool LightInspector::Draw(LightComponent& light) {
		bool changed = false;

		{
			const char* items[] = { "Directional", "Point", "Spot" };
			int current = (int)light.type;
			if (ImGui::Combo("Type", &current, items, IM_ARRAYSIZE(items))) {
				light.type = (LightType)current;
				changed = true;
			}
		}

		{
			float col[4] = {
				light.color.r / 255.0f, light.color.g / 255.0f,
				light.color.b / 255.0f, light.color.a / 255.0f
			};
			if (ImGui::ColorEdit4("Color", col)) {
				light.color.r = (unsigned char)(col[0] * 255.0f);
				light.color.g = (unsigned char)(col[1] * 255.0f);
				light.color.b = (unsigned char)(col[2] * 255.0f);
				light.color.a = (unsigned char)(col[3] * 255.0f);
				changed = true;
			}
		}

		if (ImGui::DragFloat("Intensity", &light.intensity, 0.05f, 0.0f, 100.0f, "%.2f")) {
			if (light.intensity < 0.0f) light.intensity = 0.0f;
			changed = true;
		}

		if (light.type != LightType::Point) {
			float dir[3] = { light.direction.x, light.direction.y, light.direction.z };
			if (ImGui::DragFloat3("Direction", dir, 0.01f, -1.0f, 1.0f, "%.2f")) {
				light.direction = { dir[0], dir[1], dir[2] };
				light.buildFromTransformVersion = 0;
				changed = true;
			}
		}

		if (ImGui::Checkbox("Casts Shadows", &light.castsShadows)) {
			changed = true;
		}

		if (changed) {
			++light.version;
		}
		return changed;
	}

}
