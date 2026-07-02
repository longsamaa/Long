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
		raylib::Vector3 n_tl; 
		raylib::Vector3 n_tr; 
		raylib::Vector3 n_bl; 
		raylib::Vector3 n_br; 
		raylib::Vector3 f_tl; 
		raylib::Vector3 f_tr; 
		raylib::Vector3 f_bl; 
		raylib::Vector3 f_br; 
		raylib::Vector3 up_p;
		raylib::Vector3 foward_p;
		raylib::Vector3 right_p;
	};
	
	struct LightHelperCommand {
		raylib::Vector3 origin;
		raylib::Vector3 direction;   // normalized shine direction
		raylib::Color   color{ 255, 220, 40, 255 };
		float length{ 3.0f };
	};
	using DebugCommand = std::variant<GridCommand, CameraHelperCommand, LightHelperCommand>;


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