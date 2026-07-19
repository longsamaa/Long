#include "AnimationSystem.hpp"
#include "core/Components.hpp"
#include <raylib-cpp.hpp>
#include <algorithm> // upper_bound
namespace Long {
	void Animation_FindKeys(const std::vector<float>& times, float time, size_t& i0, size_t& i1, float& f)
	{
		if (time <= times.front()) { i0 = i1 = 0; f = 0.0f; return; }
		if (time >= times.back()) { i0 = i1 = times.size() - 1; f = 0.0f; return; }
		// First key strictly greater than time; the segment starts one before it.
		i1 = std::upper_bound(times.begin(), times.end(), time) - times.begin();
		i0 = i1 - 1;
		const float t0 = times[i0], t1 = times[i1];
		f = (t1 > t0) ? (time - t0) / (t1 - t0) : 0.0f;
	}

	void AnimationSystem(entt::registry& registry, float dt) {
		auto players = registry.view<Animator>();
		for (entt::entity e : players) {
			Animator& player = players.get<Animator>(e);
			//Guard clip index
			if (player.clipIndex < 0
				|| player.clipIndex >= (int)player.clips.size()) {
				continue;
			}
			if (player.culling_mode == Animator::CullingMode::CullCompletely
				&& !player.isVisible) {
				continue;
			}
			//Guard animation clip 
			const AnimationClip& clip = player.clips[player.clipIndex];
			if (clip.channels.empty() || clip.duration <= 0.0f) {
				continue;
			}
			if (player.playing) {
				player.time += dt * player.speed;
				if (player.time > clip.duration || player.time < 0.0f) {
					if (player.loop) {
						player.time = std::fmod(player.time, clip.duration);
						if (player.time < 0.0f) player.time += clip.duration;
					}
					else {
						player.time = std::clamp(player.time, 0.0f, clip.duration);
						player.playing = false;
					}
				}
			}
			else if (!player.poseDirty) {
				continue;
			}
			if (player.culling_mode == Animator::CullingMode::UpdateTransform
				&& !player.isVisible) {
				continue;
			}
			player.poseDirty = false;
			for (const AnimationChannel& ch : clip.channels) {
				if (ch.times.empty() || ch.target == entt::null || !registry.valid(ch.target)
					|| !registry.all_of<Transform>(ch.target)) {
					continue;
				}
				size_t i0, i1; float f;
				Animation_FindKeys(ch.times, player.time, i0, i1, f);
				const bool step = (ch.interp == AnimationChannel::Interp::Step);
				switch (ch.path) {
				case AnimationChannel::Path::Translation: {
					if (i1 >= ch.vec3Keys.size()) break;
					raylib::Vector3 v = step ? ch.vec3Keys[i0]
						: raylib::Vector3(Vector3Lerp(ch.vec3Keys[i0], ch.vec3Keys[i1], f));
					registry.patch<Transform>(ch.target, [&](Transform& t) { t.position = v; });
					break;
				}
				case AnimationChannel::Path::Rotation: {
					if (i1 >= ch.quatKeys.size()) break;
					raylib::Quaternion q = step ? ch.quatKeys[i0]
						: raylib::Quaternion(QuaternionSlerp(ch.quatKeys[i0], ch.quatKeys[i1], f));
					registry.patch<Transform>(ch.target, [&](Transform& t) { t.quaternion = q; });
					break;
				}
				case AnimationChannel::Path::Scale: {
					if (i1 >= ch.vec3Keys.size()) break;
					raylib::Vector3 v = step ? ch.vec3Keys[i0]
						: raylib::Vector3(Vector3Lerp(ch.vec3Keys[i0], ch.vec3Keys[i1], f));
					registry.patch<Transform>(ch.target, [&](Transform& t) { t.scale = v; });
					break;
				}
				}
			}
		}
	}
}