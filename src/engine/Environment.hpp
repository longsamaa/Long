#pragma once
#ifndef _ENVIRONMENT_HPP_
#define _ENVIRONMENT_HPP_

#include "raylib-cpp.hpp"
#include <optional>
#include <memory>
#include "engine/camera/BaseCamera.hpp"
namespace Long {
	class SkyboxMaterial;
	class Environment {
	public:
		Environment();
		~Environment();
		void Init(class AssetManager& assets);
		void DrawSkybox(BaseCamera& camera);
		raylib::Vector3 topColor    = { 0.18f, 0.38f, 0.60f };
		raylib::Vector3 bottomColor = { 0.02f, 0.05f, 0.10f };
		float gradientSharpness = 1.0f; 
	private:
		raylib::Mesh m_skybox; 
		std::unique_ptr<SkyboxMaterial> m_skyBoxMaterial{ nullptr };
		uint32_t m_gradientShaderId = UINT32_MAX; 
		class AssetManager* m_assets = nullptr;
		bool m_ready = false;
	};

} // namespace Long

#endif // !_ENVIRONMENT_HPP_
