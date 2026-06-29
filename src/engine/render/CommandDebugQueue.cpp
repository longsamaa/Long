#include "CommandDebugQueue.hpp"
#include <raylib-cpp.hpp>
#include "rlgl.h"
namespace Long {
	void CommandDebugQueue::Submit(const DebugCommand& cmd)
	{
		m_commands.emplace_back(cmd);
	}
	const std::vector<DebugCommand>& CommandDebugQueue::getGetCommands()
	{
		return m_commands;
	}
	void CommandDebugQueue::Execute(RenderStats& stats)
	{
		for (auto& cmd : m_commands) {
			std::visit([&, stats](auto&& v) {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, GridCommand>) {
					::DrawGrid(v.slices, v.spacing);
				}
				else if constexpr (std::is_same_v<T, CameraHelperCommand>) {
					v.pos.DrawSphere(0.1f, raylib::Color::Red());

					v.pos.DrawLine3D(v.tl, raylib::Color::Green());
					v.pos.DrawLine3D(v.tr, raylib::Color::Green());
					v.pos.DrawLine3D(v.bl, raylib::Color::Green());
					v.pos.DrawLine3D(v.br, raylib::Color::Green());

					v.tl.DrawLine3D(v.tr, raylib::Color::Green());
					v.tr.DrawLine3D(v.br, raylib::Color::Green());
					v.br.DrawLine3D(v.bl, raylib::Color::Green());
					v.bl.DrawLine3D(v.tl, raylib::Color::Green());
				}
			}, cmd);
		}
	}
	void CommandDebugQueue::Clear()
	{
		m_commands.clear(); 
	}
}