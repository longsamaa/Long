#pragma once
#ifndef _COMMAND_QUEUE_HPP_
#define _COMMAND_QUEUE_HPP_
#include <vector>
#include <raylib-cpp.hpp>
#include "engine/Material.hpp"
#include "system/RenderStats.hpp"
namespace Long {
	class AssetManager;

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
		//Queue render
		void Execute(AssetManager& assets, RenderStats& stats);
	private:
		std::vector<Command> m_commands;
		std::vector<Batch> m_batches;
		// Reusable index buffer: Sort() orders these instead of swapping the heavy
		// Command structs, and BuildBatches()/Execute() walk commands through it.
		std::vector<uint32_t> m_order;
		// Pool of Batch objects kept alive across frames so their transforms vectors
		// retain capacity (BuildBatches reuses them instead of reallocating).
		size_t m_batchCount{ 0 };
	};
}
#endif // !_COMMAND_QUEUE_HPP_