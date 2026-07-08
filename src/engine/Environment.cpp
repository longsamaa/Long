#include "engine/Environment.hpp"
#include "engine/AssetManager.hpp"
#include "engine/render/GLRenderer.hpp"
#include "core/MeshCPU.hpp"
#include "rlgl.h"
#include "raymath.h"
#include "engine/Logger.hpp"
#include "engine/materials/SkyBoxMaterial.hpp"

namespace Long {
	Environment::Environment() = default;
	Environment::~Environment() = default;

	void Environment::Init(AssetManager& assets) {
		m_gradientShaderId = assets.GetShaderId("skybox_gradient");
		if (!assets.IsValidShader(m_gradientShaderId)) {
			Logger::TraceLog(::TraceLogLevel::LOG_ERROR, "skybox_gradient shader not found");
			return;
		}
		// Register the skybox cube as a normal CPU mesh asset; the backend
		// uploads it lazily like everything else. GenMeshCube also uploads its
		// own GPU copy, so unload it right after copying the CPU side.
		::Mesh cube = ::GenMeshCube(1.0f, 1.0f, 1.0f);
		m_skyboxMeshId = assets.AddMesh(MeshCPU::FromRaylib(cube));
		::UnloadMesh(cube);

		m_skyBoxMaterial = std::make_unique<SkyboxMaterial>(
			m_gradientShaderId, topColor, bottomColor, gradientSharpness);
		m_assets = &assets;
		m_ready = true;
	}

	void Environment::DrawSkybox(BaseCamera& camera, GLRenderer& gl) {
		if (!m_ready || !m_assets || !m_skyBoxMaterial) {
			return;
		}
		if (camera.Raw().projection != CAMERA_PERSPECTIVE) {
			return;
		}
		m_skyBoxMaterial->SetColor(topColor, bottomColor, gradientSharpness);
		// The backend handles state (no cull / no depth write) + the draw.
		gl.DrawSkybox(*m_assets, m_skyboxMeshId, *m_skyBoxMaterial,
			camera.Raw().GetPosition());
	}
} // namespace Long