#include "engine/render/passes/GizmoPass.hpp"
#include "engine/EditorGizmo.hpp"
#include "raylib-cpp.hpp"
#include "rlgl.h"

namespace Long {

	void GizmoPass::execute(RenderContext& ctx) {
		if (!ctx.camera || !ctx.gizmo || !ctx.gizmoTarget) {
			return;
		}
		// Gizmo input already ran in EditorState::Update; here we just draw it as
		// an overlay over the composited screen. Our gizmo uses DrawLine3D etc.,
		// so it needs to be inside the camera's 3D mode.
		ctx.camera->BeginMode();
		ctx.gizmo->Draw(*ctx.camera, *ctx.gizmoTarget);
		ctx.camera->EndMode();

		// 2D overlay (center-to-cursor line) -- must be after EndMode3D.
		ctx.gizmo->DrawScreenGuide(*ctx.camera, *ctx.gizmoTarget);
	}

}
