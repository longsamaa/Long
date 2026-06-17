#pragma once
#ifndef _CONSOLE_PANEL_HPP_
#define _CONSOLE_PANEL_HPP_
#include "../IPanel.hpp"
namespace Long {
	// Shows raylib's TraceLog output (captured by Logger) with per-level colors.
	class ConsolePanel : public IPanel {
	public:
		ConsolePanel();
		~ConsolePanel() = default;

		void render() override;
	private:
		bool m_autoScroll = true;
	};
}
#endif // !_CONSOLE_PANEL_HPP_
