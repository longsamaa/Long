#include "AnimatorInspector.hpp"
#include "core/Components.hpp"
#include "imgui.h"
#include <string>

namespace Long {

	// GLTF clips may come in unnamed; give the combo a stable fallback label.
	static std::string ClipLabel(const Animator& animator, int index) {
		if (index < 0 || index >= (int)animator.clips.size()) {
			return "(none)";
		}
		const std::string& name = animator.clips[index].name;
		return name.empty() ? ("Clip " + std::to_string(index)) : name;
	}

	bool AnimatorInspector::Draw(Animator& animator) {
		bool changed = false;

		if (animator.clips.empty()) {
			ImGui::TextDisabled("No animation clips.");
			return false;
		}

		{
			std::string preview = ClipLabel(animator, animator.clipIndex);
			if (ImGui::BeginCombo("Clip", preview.c_str())) {
				for (int i = 0; i < (int)animator.clips.size(); ++i) {
					const bool selected = (i == animator.clipIndex);
					std::string label = ClipLabel(animator, i);
					// Clip names can repeat across GLTF files; ##i keeps ids unique.
					label += "##" + std::to_string(i);
					if (ImGui::Selectable(label.c_str(), selected)) {
						animator.clipIndex = i;
						animator.time = 0.0f;
						animator.poseDirty = true; // show the new clip's first frame even while paused
						changed = true;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}

		const bool validClip = animator.clipIndex >= 0
			&& animator.clipIndex < (int)animator.clips.size();
		const float duration = validClip
			? animator.clips[animator.clipIndex].duration : 0.0f;

		if (ImGui::Button(animator.playing ? "Pause" : "Play")) {
			animator.playing = !animator.playing;
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop")) {
			animator.playing = false;
			animator.time = 0.0f;
			animator.poseDirty = true; // snap the pose back to frame 0
			changed = true;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%.2fs / %.2fs", animator.time, duration);

		// Scrubber: the pose follows the drag even while paused (poseDirty asks
		// AnimationSystem for a one-shot resample). While playing it doubles as
		// a live playhead.
		if (ImGui::SliderFloat("Time", &animator.time, 0.0f, duration, "%.2fs")) {
			animator.poseDirty = true;
			changed = true;
		}

		if (ImGui::DragFloat("Speed", &animator.speed, 0.05f, -10.0f, 10.0f, "%.2fx")) {
			changed = true;
		}
		if (ImGui::Checkbox("Loop", &animator.loop)) {
			changed = true;
		}

		{
			const char* items[] = { "Always Animate", "Cull Update Transforms", "Cull Completely" };
			int current = (int)animator.culling_mode;
			if (ImGui::Combo("Culling", &current, items, IM_ARRAYSIZE(items))) {
				animator.culling_mode = (Animator::CullingMode)current;
				changed = true;
			}
		}

		return changed;
	}

}
