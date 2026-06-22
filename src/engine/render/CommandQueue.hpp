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
	};
}
#endif // !_COMMAND_QUEUE_HPP_