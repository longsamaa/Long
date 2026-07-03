#pragma once
#ifndef _LIGHT_INSPECTOR_HPP_
#define _LIGHT_INSPECTOR_HPP_
namespace Long {
	struct LightComponent;
	class LightInspector {
	public:
		static bool Draw(LightComponent& light);
	};
}
#endif // !_LIGHT_INSPECTOR_HPP_
