#pragma once
#ifndef _GL_RENDERER_HPP_
#define _GL_RENDERER_HPP_
#include <vector>
#include <string>
#include <unordered_map>
#include <raylib-cpp.hpp>
#include "system/RenderStats.hpp"
#include <optional>

namespace Long {
	class AssetManager;
	class BaseMaterial;
	class MeshCPU;
	struct SceneLights;
	struct Batch;
	struct Skeleton;
	struct GLSkinMesh {
		uint32_t joints_vbo;
		uint32_t weights_vbo;
	};
	struct GLGpuMesh {
		::Mesh mesh; //Mesh raylib
		std::optional<GLSkinMesh> skin; //Skin mesh raylib
	};
	class GLRenderer {
	public:
		GLRenderer() = default;
		~GLRenderer();                       // releases the instance VBO + GPU meshes
		GLRenderer(const GLRenderer&) = delete;
		GLRenderer& operator=(const GLRenderer&) = delete;
		void DrawBatches(const std::vector<Batch>& batches, size_t batchCount,
			AssetManager& assets, RenderStats& stats, const SceneLights* lights);
		void DrawDepth(const std::vector<Batch>& batches, size_t batchCount,
			AssetManager& assets, const raylib::Matrix& lightViewProj,
			uint32_t depthShaderId, const float& range, bool linearDistance = false,
			const raylib::Vector3* lightPos = nullptr);
		void ApplyMaterial(const BaseMaterial& material, const ::Shader& shader);
		void DrawMeshImmediate(AssetManager& assets, uint32_t meshId,
			const BaseMaterial& material, const raylib::Matrix& transform,
			const Skeleton* skeleton = nullptr);
		void DrawSkybox(AssetManager& assets, uint32_t meshId,
			const BaseMaterial& material, const raylib::Vector3& cameraPos);
		void DrawDebugLines(AssetManager& assets,
			const std::vector<struct DebugLineVertex>& lines);
		::Mesh uploadRaylibMesh(const MeshCPU& cpu);
	private:
		GLGpuMesh& UploadGpuMesh(AssetManager& assets, uint32_t meshId);
		std::optional<GLSkinMesh> uploadSkinMesh(uint32_t vaoId, const MeshCPU& cpu);
		void UploadInstanceTransforms(const std::vector<raylib::Matrix>& transforms);
		void BindJointMatrices(const Skeleton& skel);
		std::vector<GLGpuMesh> m_gpuMeshes;      // GPU cache, index = asset mesh id
		unsigned int m_instanceVbo{ 0 };      // persistent instance-transform VBO
		size_t m_instanceCapacity{ 0 };       // capacity of m_instanceVbo, in instances
		std::vector<float> m_instanceStaging;  // CPU staging: 16 floats per instance
		unsigned int m_lineVao{ 0 };
		unsigned int m_lineVbo{ 0 };
		size_t m_lineCapacity{ 0 };           // capacity in vertices
		unsigned int m_jointSsbo{ 0 };        // joint matrices SSBO (skinning)
		size_t m_jointSsboCapacity{ 0 };      // capacity in bytes
		std::vector<float> m_jointStaging;    // CPU staging: 16 floats per joint
		std::unordered_map<unsigned int, std::unordered_map<std::string, int>> m_materialLocs;
		::Material m_immediateMaterial{};
		bool m_immediateMaterialReady{ false };
	};
}
#endif // !_GL_RENDERER_HPP_