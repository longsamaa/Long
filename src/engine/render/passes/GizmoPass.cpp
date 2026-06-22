#include "engine/render/passes/GizmoPass.hpp"
#include "engine/EditorGizmo.hpp"
#include "raylib-cpp.hpp"
#include "rlgl.h"

namespace Long {
	void GizmoPass::execute(RenderContext& ctx) {
		if (!ctx.camera || !ctx.gizmo || !ctx.gizmoTarget) {
			return;
		}
		ctx.camera->BeginMode();
		ctx.gizmo->Draw(*ctx.camera, *ctx.gizmoTarget);
		ctx.camera->EndMode();
		ctx.gizmo->DrawScreenGuide(*ctx.camera, *ctx.gizmoTarget);
	}
}