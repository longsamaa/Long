#pragma once
#ifndef _GIZMO_PASS_HPP_
#define _GIZMO_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
namespace Long {

	// Draws the transform gizmo as an OVERLAY directly to the screen, AFTER the
	// whole pipeline (FXAA blitted the scene). Drawing it here -- not inside the
	// scene RenderTexture -- keeps the camera matrices aligned with the real
	// window, so the gizmo's mouse-ray picking (axes + planes) is accurate.
	class GizmoPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;
	};

}
#endif // !_GIZMO_PASS_HPP_
