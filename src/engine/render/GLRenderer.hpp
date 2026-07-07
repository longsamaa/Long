#pragma once
#ifndef _GL_RENDERER_HPP_
#define _GL_RENDERER_HPP_
#include <vector>
#include <raylib-cpp.hpp>
#include "system/RenderStats.hpp"

namespace Long {
	class AssetManager;
	struct SceneLights;
	struct Batch;
	class GLRenderer {
	public:
		GLRenderer() = default;
		~GLRenderer();                       // releases the instance VBO
		GLRenderer(const GLRenderer&) = delete;
		GLRenderer& operator=(const GLRenderer&) = delete;

		void DrawBatches(const std::vector<Batch>& batches, size_t batchCount,
			AssetManager& assets, RenderStats& stats, const SceneLights* lights);

		void DrawDepth(const std::vector<Batch>& batches, size_t batchCount,
			AssetManager& assets, const raylib::Matrix& lightViewProj,
			uint32_t depthShaderId, const float& range, bool linearDistance = false,
			const raylib::Vector3* lightPos = nullptr);

	private:
		void UploadInstanceTransforms(const std::vector<raylib::Matrix>& transforms);
		unsigned int m_instanceVbo{ 0 };      // persistent instance-transform VBO
		size_t m_instanceCapacity{ 0 };       // capacity of m_instanceVbo, in instances
		std::vector<float> m_instanceStaging;  // CPU staging: 16 floats per instance
	};
}
#endif // !_GL_RENDERER_HPP_
