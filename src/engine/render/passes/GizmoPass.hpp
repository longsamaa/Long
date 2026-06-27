#pragma once
#ifndef _GIZMO_PASS_HPP_
#define _GIZMO_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
namespace Long {
	class GizmoPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	};

}
#endif // !_GIZMO_PASS_HPP_
