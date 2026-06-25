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
		uint32_t stageCount = 0; //number of state changes (shader/material switches)
		uint32_t materialCount = 0; //number of material (shader/material)
		uint32_t renderPassCalls = 0; //number of render pass
		// Per-stage CPU timings (milliseconds), filled by ScenePass each frame.
		double msRenderSystem = 0.0; // build command list from the ECS view
		double msSort = 0.0;         // sort the command index buffer
		double msBuildBatches = 0.0; // group sorted commands into batches
		double msExecute = 0.0;      // issue draw calls (DrawMeshInstanced etc.)
		double msScenePass = 0.0;    // whole ScenePass (clear -> unbind)
		// Update-phase timings (filled by EditorState::Update, not the render passes).
		double msTransformSystem = 0.0; // world-matrix update for all transforms
		double msPicking = 0.0;         // mouse raycast against colliders
		double msUpdate = 0.0;          // whole EditorState::Update
		// Per-pass timings (ms), indexed by pass order; filled by Renderer::Render.
		static constexpr int kMaxPasses = 8;
		double msPass[kMaxPasses] = {};
		int passCount = 0;              // how many entries in msPass are valid
		double msRenderTotal = 0.0;     // whole Renderer::Render (all passes)
		double msEndDrawing = 0.0;      // EndDrawing/SwapBuffers (incl. vsync wait)
		void Reset() { *this = RenderStats{}; }
	};
}
#endif // !_RENDER_STATS_HPP_