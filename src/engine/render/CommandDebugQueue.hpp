#pragma once
#ifndef _COMMAND_DEBUG_QUEUE_HPP_
#define _COMMAND_DEBUG_QUEUE_HPP_
#include <vector>
#include <variant>
#include <system/RenderStats.hpp>
#include <raylib-cpp.hpp>

namespace Long {
	struct GridCommand {
		uint32_t slices{ 0 };
		float_t spacing{ 0.0 };
	}; 
	struct CameraHelperCommand {
		raylib::Vector3 pos; 
		raylib::Vector3 tl; 
		raylib::Vector3 tr; 
		raylib::Vector3 bl; 
		raylib::Vector3 br; 
	};
	using DebugCommand = std::variant<GridCommand,CameraHelperCommand>;


	class CommandDebugQueue {
	public:
		CommandDebugQueue() = default;
		~CommandDebugQueue() = default;
		CommandDebugQueue(const CommandDebugQueue&) = delete;
		CommandDebugQueue& operator=(const CommandDebugQueue&) = delete;
	public:
		void Submit(const DebugCommand& cmd);
		const std::vector<DebugCommand>& getGetCommands();
		void Execute(RenderStats& stats);
		void Clear(); 
	private:
		std::vector<DebugCommand> m_commands;
	};
}
#endif // !_COMMAND_DEBUG_QUEUE_HPP_