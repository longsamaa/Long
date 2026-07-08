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

	// One vertex of a debug line (GL_LINES: 2 vertices per segment).
	// 3 floats + 4 normalized bytes = 16 bytes, matches the VAO layout that
	// GLRenderer::DrawDebugLines builds (loc 0 = position, loc 3 = color).
	struct DebugLineVertex {
		float x, y, z;
		unsigned char r, g, b, a;
	};

	// Pure CPU side of debug drawing: collects helper commands and flattens them
	// into a line vertex list. Knows NOTHING about GL -- the backend
	// (GLRenderer::DrawDebugLines) uploads and draws the list.
	class CommandDebugQueue {
	public:
		CommandDebugQueue() = default;
		~CommandDebugQueue() = default;
		CommandDebugQueue(const CommandDebugQueue&) = delete;
		CommandDebugQueue& operator=(const CommandDebugQueue&) = delete;
	public:
		void Submit(const DebugCommand& cmd);
		const std::vector<DebugCommand>& getGetCommands();
		// Flatten every submitted command into m_lines (grid -> line grid, camera
		// frustum -> edges, light -> rays, spheres -> 3 axis circles).
		void BuildLines(RenderStats& stats);
		const std::vector<DebugLineVertex>& Lines() const { return m_lines; }
		void Clear();
	private:
		void AddLine(const raylib::Vector3& a, const raylib::Vector3& b, const raylib::Color& c);
		void AddCircle(const raylib::Vector3& center, float radius, int axis, const raylib::Color& c);
		std::vector<DebugCommand> m_commands;
		std::vector<DebugLineVertex> m_lines;
	};
}
#endif // !_COMMAND_DEBUG_QUEUE_HPP_