#pragma once
#ifndef _RENDER_CONTEXT_HPP_
#define _RENDER_CONTEXT_HPP_
#include <entt/entt.hpp>
#include <raylib-cpp.hpp>
#include <vector>
#include <any>
#include "system/RenderStats.hpp"
#include "RenderTarget.hpp"
#include "CommandQueue.hpp"
#include "CommandDebugQueue.hpp"
#include "core/Components.hpp"   
#include "engine/camera/BaseCamera.hpp"
namespace Long {
	class AssetManager;
	class Environment;
	class FrustumCulling;
	struct RenderContext {
		Environment* environment{ nullptr }; // scene skybox / lighting
		CommandQueue* commandQueue{ nullptr };
		CommandDebugQueue* commandDebugQueue{ nullptr }; 
		entt::registry* registry{ nullptr };
		AssetManager* assets{ nullptr };     // ScenePass needs meshes/materials
		BaseCamera* camera{ nullptr };
		FrustumCulling* frustum{ nullptr };  // visibility: cull entities outside view
		uint32_t width{ 0 };
		uint32_t height{ 0 };
		RenderTarget* sceneTarget{ nullptr };  // 3D scene color
		RenderTarget* maskTarget{ nullptr };   // selected-only mask (outline)
		RenderTarget* finalTarget{ nullptr };  // composited image (FXAA reads this)
		RenderTarget* brightTarget{ nullptr };  // bright-pass output (bloom/flare read this)
		RenderTarget* blurTarget{ nullptr };    // blur scratch / result (separable blur)
		RenderTarget* ldrTarget{ nullptr };     // tonemapped LDR image (FXAA reads this)
		RenderStats renderStats;
		std::vector<entt::entity> selectedEntities{};

		// Gizmo overlay: the gizmo and the selected entity's transform (or null).
		class EditorGizmo* gizmo{ nullptr };
		Transform* gizmoTarget{ nullptr };
	};
}
#endif // !_RENDER_CONTEXT_HPP_