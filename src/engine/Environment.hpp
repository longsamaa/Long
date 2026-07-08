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
