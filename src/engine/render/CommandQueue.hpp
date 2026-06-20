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
		//Queue render
		void Execute(AssetManager& assets, RenderStats& stats);
	private:
		std::vector<Command> m_commands;
	};
}
#endif // !_COMMAND_QUEUE_HPP_