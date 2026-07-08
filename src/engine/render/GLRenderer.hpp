#pragma once
#ifndef _GL_RENDERER_HPP_
#define _GL_RENDERER_HPP_
#include <vector>
#include <string>
#include <unordered_map>
#include <raylib-cpp.hpp>
#include "system/RenderStats.hpp"

namespace Long {
	class AssetManager;
	class BaseMaterial;
	struct SceneLights;
	struct Batch;
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

		// Apply a CPU material to the CURRENTLY BOUND shader program: resolve each
		// named uniform via the backend's own location cache and upload it. This
		// is the GL half of BaseMaterial -- a Vulkan backend would translate the
		// same table into descriptor/push-constant writes instead.
		void ApplyMaterial(const BaseMaterial& material, const ::Shader& shader);

		// Immediate single-mesh draw (MaskPass...): resolves mesh id + material
		// through the backend and issues one raylib DrawMesh.
		void DrawMeshImmediate(AssetManager& assets, uint32_t meshId,
			const BaseMaterial& material, const raylib::Matrix& transform);

		// Skybox: draw `meshId` centered on the camera with backface culling off
		// and depth writes off (RAII scopes). Call inside the camera's BeginMode.
		void DrawSkybox(AssetManager& assets, uint32_t meshId,
			const BaseMaterial& material, const raylib::Vector3& cameraPos);

		// Draw the debug helper lines (CommandDebugQueue::Lines()) with the
		// debug_line shader as one GL_LINES call. Call inside the camera's
		// BeginMode (matrices are read from rlgl). Owns a persistent dynamic VBO.
		void DrawDebugLines(AssetManager& assets,
			const std::vector<struct DebugLineVertex>& lines);

	private:
		// Assets only store MeshCPU; the backend owns the GPU side. Lazily uploads
		// mesh `meshId` on first use and caches it (indexed by id). Returns a mesh
		// with vaoId == 0 if the id is invalid / upload failed.
		::Mesh& GetGpuMesh(AssetManager& assets, uint32_t meshId);

		void UploadInstanceTransforms(const std::vector<raylib::Matrix>& transforms);
		std::vector<::Mesh> m_gpuMeshes;      // GPU cache, index = asset mesh id
		unsigned int m_instanceVbo{ 0 };      // persistent instance-transform VBO
		size_t m_instanceCapacity{ 0 };       // capacity of m_instanceVbo, in instances
		std::vector<float> m_instanceStaging;  // CPU staging: 16 floats per instance
		// Debug-line rendering (grid / helpers): persistent VAO + dynamic VBO.
		unsigned int m_lineVao{ 0 };
		unsigned int m_lineVbo{ 0 };
		size_t m_lineCapacity{ 0 };           // capacity in vertices
		// Per-(program, uniform-name) location cache for ApplyMaterial. Locations
		// are a GL concept, so the cache lives HERE, not in the material asset.
		std::unordered_map<unsigned int, std::unordered_map<std::string, int>> m_materialLocs;
		// Scratch raylib material for DrawMeshImmediate (DrawMesh needs one);
		// created lazily, shader swapped per call.
		::Material m_immediateMaterial{};
		bool m_immediateMaterialReady{ false };
	};
}
#endif // !_GL_RENDERER_HPP_
