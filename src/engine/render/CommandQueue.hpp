#pragma once
#ifndef _COMMAND_QUEUE_HPP_
#define _COMMAND_QUEUE_HPP_
#include <vector>
#include <raylib-cpp.hpp>
#include "engine/Material.hpp"

namespace Long {
	enum class CommandType {
		Mesh,
		Grid
	};

	// Draw command references the mesh by ASSET ID, not by pointer: assets only
	// hold CPU data (MeshCPU); the GL backend resolves the id to its cached GPU
	// mesh at draw time.
	struct Command {
		raylib::Matrix worldMatrix;
		uint32_t meshId{ UINT32_MAX };
		BaseMaterial* material{ nullptr };
		bool isCulled{ false };
	};

	// A group of draws sharing the same (mesh, material) -- i.e. everything but the
	// transform. Such a group can be drawn in a single instanced call. Consumed by
	// GLRenderer, which turns it into GL draw calls.
	struct Batch {
		uint32_t meshId{ UINT32_MAX };
		BaseMaterial* material{ nullptr };
		std::vector<raylib::Matrix> transforms;
	};

	// Pure CPU-side draw list: collect commands, sort them for state coherence,
	// and group them into batches. It knows NOTHING about OpenGL -- GLRenderer
	// (or another backend) consumes batches() and issues the actual draw calls.
	class CommandQueue {
	public:
		CommandQueue() = default;
		~CommandQueue() = default;
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

		// Batches produced by BuildBatches(). Only the first batchCount() entries
		// are valid (the vector is reused across frames, not shrunk).
		const std::vector<Batch>& batches() const { return m_batches; }
		size_t batchCount() const { return m_batchCount; }

	private:
		std::vector<Command> m_commands;
		std::vector<Batch> m_batches;
		std::vector<uint32_t> m_order;
		size_t m_batchCount{ 0 };
	};
}
#endif // !_COMMAND_QUEUE_HPP_
