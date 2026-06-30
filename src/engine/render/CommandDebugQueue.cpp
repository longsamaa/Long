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
					v.pos.DrawSphere(0.3f, raylib::Color::Red());

					v.pos.DrawLine3D(v.n_tl, raylib::Color::Green());
					v.pos.DrawLine3D(v.n_tr, raylib::Color::Green());
					v.pos.DrawLine3D(v.n_bl, raylib::Color::Green());
					v.pos.DrawLine3D(v.n_br, raylib::Color::Green());

					v.n_tl.DrawLine3D(v.n_tr, raylib::Color::Green());
					v.n_tr.DrawLine3D(v.n_br, raylib::Color::Green());
					v.n_br.DrawLine3D(v.n_bl, raylib::Color::Green());
					v.n_bl.DrawLine3D(v.n_tl, raylib::Color::Green());

					v.n_tl.DrawLine3D(v.f_tl, raylib::Color::Green());
					v.n_tr.DrawLine3D(v.f_tr, raylib::Color::Green());
					v.n_br.DrawLine3D(v.f_br, raylib::Color::Green());
					v.n_bl.DrawLine3D(v.f_bl, raylib::Color::Green());

					v.f_tl.DrawLine3D(v.f_tr, raylib::Color::Green());
					v.f_tr.DrawLine3D(v.f_br, raylib::Color::Green());
					v.f_br.DrawLine3D(v.f_bl, raylib::Color::Green());
					v.f_bl.DrawLine3D(v.f_tl, raylib::Color::Green());

					//draw axis
					v.pos.DrawLine3D(v.up_p,raylib::Color::Green()); 
					v.pos.DrawLine3D(v.right_p,raylib::Color::Red()); 
					v.pos.DrawLine3D(v.foward_p,raylib::Color::Blue());
				}
			}, cmd);
		}
	}
	void CommandDebugQueue::Clear()
	{
		m_commands.clear(); 
	}
}