#pragma once
#ifndef _RENDER_STATS_HPP_
#define _RENDER_STATS_HPP_
#include <cstdint>

namespace Long {
	// Per-frame rendering counters, filled by RenderSystem. raylib does not
	// expose draw-call / triangle counts, so we tally them ourselves while
	// drawing. Reset at the start of each RenderSystem pass.
	struct RenderStats {
		uint32_t drawCalls = 0;     // number of DrawMesh calls
		uint32_t triangles = 0;     // total triangles submitted
		uint32_t vertices = 0;      // total vertices submitted
		uint32_t culledEntities = 0; // entities skipped (invisible / invalid)

		void Reset() { *this = RenderStats{}; }
	};
}
#endif // !_RENDER_STATS_HPP_
