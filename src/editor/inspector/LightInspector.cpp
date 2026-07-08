#include "LightInspector.hpp"
#include "core/Components.hpp"
#include "core/math/transform.hpp" // KelvinToRGB (temperature preview swatch)
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

		// Color temperature: warm tungsten (~2700K) .. neutral (6500K) .. cool
		// sky (~10000K+). Swatch previews the blackbody tint being applied.
		if (ImGui::SliderFloat("Temperature", &light.temperature, 1000.0f, 12000.0f, "%.0f K")) {
			changed = true;
		}
		raylib::Vector3 kelvin = KelvinToRGB(light.temperature);
		ImGui::SameLine();
		ImGui::ColorButton("##kelvinPreview",
			ImVec4(kelvin.x, kelvin.y, kelvin.z, 1.0f),
			ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker);

		if (light.type != LightType::Point) {
			float dir[3] = { light.direction.x, light.direction.y, light.direction.z };
			if (ImGui::DragFloat3("Direction", dir, 0.01f, -1.0f, 1.0f, "%.2f")) {
				light.direction = { dir[0], dir[1], dir[2] };
				light.buildFromTransformVersion = 0;
				changed = true;
			}
		}

		if (light.type != LightType::Directional) {
			// Reach of point/spot: light fades to zero at this distance.
			if (ImGui::DragFloat("Range", &light.range, 0.5f, 0.1f, 1000.0f, "%.1f")) {
				if (light.range < 0.1f) light.range = 0.1f;
				changed = true;
			}
		}

		if (light.type == LightType::Spot) {
			// Half-angles in degrees. Keep inner < outer so the rim stays soft
			// (LightSystem clamps too, but the UI shouldn't let them cross).
			if (ImGui::DragFloat("Inner Angle", &light.innerAngle, 0.5f, 0.0f, 89.0f, "%.1f deg")) {
				if (light.innerAngle < 0.0f) light.innerAngle = 0.0f;
				if (light.outerAngle < light.innerAngle) light.outerAngle = light.innerAngle;
				changed = true;
			}
			if (ImGui::DragFloat("Outer Angle", &light.outerAngle, 0.5f, 0.0f, 90.0f, "%.1f deg")) {
				if (light.outerAngle < light.innerAngle) light.outerAngle = light.innerAngle;
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
