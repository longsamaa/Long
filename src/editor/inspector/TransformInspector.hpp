#pragma once
#ifndef _TRANSFORM_INSPECTOR_HPP_
#define _TRANSFORM_INSPECTOR_HPP_
namespace Long {
	struct Transform;
	class TransformInspector {
	public:
		static bool Draw(Transform& param);
	};
}
#endif // !_TRANSFORM_INSPECTOR_HPP_
