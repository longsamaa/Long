#pragma once
#ifndef _PROFILER_PANEL_HPP_
#define _PROFILER_PANEL_HPP_
#include "editor/IPanel.hpp"
#include "system/RenderStats.hpp"
namespace Long {
	// Shows per-frame render stats (draw calls, triangles, FPS). It does not own
	// the stats -- it points at the RenderStats that RenderSystem fills each
	// frame, so it always shows the latest values.
	class ProfilerPanel : public IPanel {
	public:
		explicit ProfilerPanel(const RenderStats& stats);
		~ProfilerPanel() = default;

		void render() override;
	private:
		const RenderStats& m_stats;
	};
}
#endif // !_PROFILER_PANEL_HPP_
