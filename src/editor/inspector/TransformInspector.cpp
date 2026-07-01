#include "TransformInspector.hpp"
#include "core/Components.hpp"
#include "imgui.h"
#include "raymath.h"

namespace Long {
	bool TransformInspector::Draw(Transform& t) {
		bool changed = false;

		{
			raylib::Vector3 pos = t.getPos();
			if (ImGui::DragFloat3("Position", &pos.x, 0.05f, 0.0f, 0.0f, "%.3f")) {
				t.setPos(pos);
				changed = true;
			}
		}

		{
			raylib::Vector3 euler = QuaternionToEuler(t.getQuaternion()); // radians
			raylib::Vector3 deg{ euler.x * RAD2DEG, euler.y * RAD2DEG, euler.z * RAD2DEG };
			if (ImGui::DragFloat3("Rotation", &deg.x, 0.5f, 0.0f, 0.0f, "%.2f")) {
				raylib::Quaternion q = raylib::Quaternion(QuaternionFromEuler(deg.x * DEG2RAD,
														   deg.y * DEG2RAD,
														   deg.z * DEG2RAD));
				t.setQuaternion(q);
				changed = true;
			}
		}

		{
			raylib::Vector3 scale = t.getScale();
			if (ImGui::DragFloat3("Scale", &scale.x, 0.05f, 0.0f, 0.0f, "%.3f")) {
				t.setScale(scale);
				changed = true;
			}
		}

		return changed;
	}

}
