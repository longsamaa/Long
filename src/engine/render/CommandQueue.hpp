#pragma once
#ifndef _COMMAND_QUEUE_HPP_
#define _COMMAND_QUEUE_HPP_
#include <vector>
#include <raylib-cpp.hpp>
#include "engine/Material.hpp"
#include "system/RenderStats.hpp"


namespace Long {
	class AssetManager;
	struct SceneLights; 
	enum class CommandType {
		Mesh,
		Grid
	};

	struct Command {
		raylib::Matrix worldMatrix;
		raylib::Mesh* mesh{ nullptr };
		BaseMaterial* material{ nullptr };
		bool isCulled{ false };
	};

	// A group of draws sharing the same (mesh, material) -- i.e. everything but the
	// transform. Such a group can be drawn in a single instanced call.
	struct Batch {
		raylib::Mesh* mesh{ nullptr };
		BaseMaterial* material{ nullptr };
		std::vector<raylib::Matrix> transforms;
	};

	class CommandQueue {
	public:
		CommandQueue() = default;
		~CommandQueue(); // releases the persistent instance-transform VBO
		//No copy
		CommandQueue(const CommandQueue&) = delete;
		CommandQueue& operator=(const CommandQueue&) = delete;
	public:
		void Submit(const Command& cmd);
		// Pre-grow the command buffer so a frame's Submit() calls don't realloc.
		void Reserve(size_t count);
		void Clear();
		const std::vector<Command>& GetCommands();
		//Sort de cache state changes
		void Sort();
		// Group sorted commands into batches by (mesh, material). Call after Sort().
		void BuildBatches();
		//Queue render
		void Execute(AssetManager& assets, RenderStats& stats, const SceneLights* lights);
		// Draw the SAME batches as Execute but depth-only, from a light's view, into
		// the currently bound framebuffer (a ShadowMap). Uses the shadow_depth shader
		// (and its instanced variant) instead of each material's shader; lightViewProj
		// replaces the camera matrices. Call after BuildBatches().
		void ExecuteDepth(AssetManager& assets,
			const raylib::Matrix& lightViewProj,
			uint32_t depthShaderId,
			const float& range, 
			bool linearDistance = false);
	private:
		// Upload a batch's transforms into the persistent instance VBO (grown as
		// needed). raylib's DrawMeshInstanced allocates and frees a fresh VBO on
		// EVERY call; we keep one alive across frames and just update it.
		void UploadInstanceTransforms(const std::vector<raylib::Matrix>& transforms);

		std::vector<Command> m_commands;
		std::vector<Batch> m_batches;
		std::vector<uint32_t> m_order;
		size_t m_batchCount{ 0 };
		unsigned int m_instanceVbo{ 0 };    // persistent instance-transform VBO
		size_t m_instanceCapacity{ 0 };     // capacity of m_instanceVbo, in instances
		std::vector<float> m_instanceStaging; // CPU staging: 16 floats per instance
	};
}
#endif // !_COMMAND_QUEUE_HPP_