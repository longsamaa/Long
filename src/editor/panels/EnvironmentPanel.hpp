#pragma once
#ifndef _ENVIRONMENT_PANEL_HPP_
#define _ENVIRONMENT_PANEL_HPP_
#include "editor/IPanel.hpp"
namespace Long {
	class Environment;

	// Edits the scene Environment: skybox gradient colors + horizon sharpness.
	// Holds a reference to the Environment (does not own it).
	class EnvironmentPanel : public IPanel {
	public:
		explicit EnvironmentPanel(Environment& env);
		~EnvironmentPanel() = default;

		void render() override;
	private:
		Environment& m_env;
	};
}
#endif // !_ENVIRONMENT_PANEL_HPP_
