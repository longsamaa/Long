#pragma once 
#ifndef _BASE_PANEL_HPP_
#define _BASE_PANEL_HPP_
#include <string>
namespace Long {
	//Base class for imgui panels
	class IPanel {
	public:
		virtual ~IPanel() = default;
		virtual void render() = 0;
		virtual void open() { m_isOpen = true; }
		virtual void close() { m_isOpen = false; }

		// For driving the panel from a menu / list.
		bool& isOpen() { return m_isOpen; }
		const std::string& title() const { return m_title; }
	protected:
		bool m_isOpen{ false };
		std::string m_title;
	};
}
#endif // !_BASE_PANEL_HPP_
