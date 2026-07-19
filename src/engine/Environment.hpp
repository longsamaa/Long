#pragma once
#ifndef _ENVIRONMENT_HPP_
#define _ENVIRONMENT_HPP_

#include "raylib-cpp.hpp"
#include <optional>
#include <memory>
#include "engine/camera/BaseCamera.hpp"
namespace Long {
	class SkyboxMaterial;
	class GLRenderer;
	class Environment {
	public:
		Environment();
		~Environment();
		void Init(class AssetManager& assets);
		// Needs the GL backend: materials are pure CPU data now, the backend
		// resolves + uploads their uniforms (GLRenderer::ApplyMaterial).
		void DrawSkybox(BaseCamera& camera, GLRenderer& gl);
		raylib::Vector3 topColor    = { 0.18f, 0.38f, 0.60f };
		raylib::Vector3 bottomColor = { 0.02f, 0.05f, 0.10f };
		float gradientSharpness = 1.0f;
		// Bloom tuning -- read every frame by BrightPass / BloomCompositePass
		// through RenderContext::environment, editable in EnvironmentPanel.
		float bloomThreshold = 1.5f;  // luma where the knee band ends (>1 = HDR only)
		float bloomSoftKnee = 0.3f;   // 0 = hard cut, ~0.5 = wide soft roll-off
		float bloomClampMax = 4.0f;   // firefly clamp: max luma fed into the blur
		float bloomStrength = 0.6f;   // how much blurred bloom is added back
	private:
		// The cube lives in the AssetManager as MeshCPU like every other mesh;
		// the GL backend uploads/caches its GPU side.
		uint32_t m_skyboxMeshId = UINT32_MAX;
		std::unique_ptr<SkyboxMaterial> m_skyBoxMaterial{ nullptr };
		uint32_t m_gradientShaderId = UINT32_MAX;
		class AssetManager* m_assets = nullptr;
		bool m_ready = false;
	};

} // namespace Long

#endif // !_ENVIRONMENT_HPP_
