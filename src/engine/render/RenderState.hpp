#pragma once
#ifndef _RENDER_STATE_HPP_
#define _RENDER_STATE_HPP_
#include "rlgl.h"

// RAII scope guards for OpenGL (rlgl) render state. Set a state on the way in, and
// restore the PREVIOUS value on the way out -- so passes never leak state or forget
// to reset it (which BloomPass/Environment/gizmo were doing by hand). Use like:
//
//     {
//         Long::ScopedBlend blend(RL_BLEND_ADDITIVE);
//         Long::ScopedDepthTest depth(false);
//         ... draw ...
//     } // state restored here
//
// These wrap rlgl (not raw GL) so they stay consistent with raylib's own tracking.
namespace Long {

	// Blend mode (RL_BLEND_ALPHA, RL_BLEND_ADDITIVE, ...). rlgl keeps the current
	// blend mode in its private RLGL.State with no getter, so we can't read the
	// prior value -- we restore to RL_BLEND_ALPHA (raylib's default) on exit.
	class ScopedBlend {
	public:
		explicit ScopedBlend(int mode) { rlSetBlendMode(mode); }
		~ScopedBlend() { rlSetBlendMode(RL_BLEND_ALPHA); } // restore default
		ScopedBlend(const ScopedBlend&) = delete;
		ScopedBlend& operator=(const ScopedBlend&) = delete;
	};

	// Depth testing on/off. rlgl has no getter, so we assume depth test is ON by
	// default (raylib's normal state) and restore to that.
	class ScopedDepthTest {
	public:
		explicit ScopedDepthTest(bool enabled) {
			if (enabled) rlEnableDepthTest(); else rlDisableDepthTest();
		}
		~ScopedDepthTest() { rlEnableDepthTest(); } // restore default (ON)
		ScopedDepthTest(const ScopedDepthTest&) = delete;
		ScopedDepthTest& operator=(const ScopedDepthTest&) = delete;
	};

	// Writing to the depth buffer on/off. Default is ON.
	class ScopedDepthMask {
	public:
		explicit ScopedDepthMask(bool enabled) {
			if (enabled) rlEnableDepthMask(); else rlDisableDepthMask();
		}
		~ScopedDepthMask() { rlEnableDepthMask(); } // restore default (ON)
		ScopedDepthMask(const ScopedDepthMask&) = delete;
		ScopedDepthMask& operator=(const ScopedDepthMask&) = delete;
	};

	// Backface culling on/off. Default is ON.
	class ScopedBackfaceCull {
	public:
		explicit ScopedBackfaceCull(bool enabled) {
			if (enabled) rlEnableBackfaceCulling(); else rlDisableBackfaceCulling();
		}
		~ScopedBackfaceCull() { rlEnableBackfaceCulling(); } // restore default (ON)
		ScopedBackfaceCull(const ScopedBackfaceCull&) = delete;
		ScopedBackfaceCull& operator=(const ScopedBackfaceCull&) = delete;
	};

}
#endif // !_RENDER_STATE_HPP_
