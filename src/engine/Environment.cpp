#include "engine/Environment.hpp"
#include "engine/AssetManager.hpp"
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
		m_skybox = ::GenMeshCube(1.0f, 1.0f, 1.0f);
		m_skyBoxMaterial = std::make_unique<SkyboxMaterial>(
			m_gradientShaderId, topColor, bottomColor, gradientSharpness);
		m_assets = &assets;
		m_ready = true;
	}

	void Environment::DrawSkybox(BaseCamera& camera) {
		if (!m_ready || !m_assets || !m_skyBoxMaterial) {
			return;
		}
		if (camera.Raw().projection != CAMERA_PERSPECTIVE) {
			return;
		}
		m_skyBoxMaterial->SetColor(topColor, bottomColor, gradientSharpness);
		raylib::Shader& shader = m_assets->GetShader(m_gradientShaderId);
		raylib::Material& rlMat = m_skyBoxMaterial->Apply(shader);
		rlDisableBackfaceCulling();
		rlDisableDepthMask();
		m_skybox.Draw(rlMat, MatrixTranslate(camera.Raw().GetPosition().x, camera.Raw().GetPosition().y, camera.Raw().GetPosition().z));
		rlEnableBackfaceCulling();
		rlEnableDepthMask();
	}

} // namespace Long
