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
				}
				else if constexpr (std::is_same_v<T, LightHelperCommand>) {
					raylib::Vector3 dir = raylib::Vector3(v.direction).Normalize();
					raylib::Vector3 end = raylib::Vector3(v.origin).Add(dir.Scale(v.length));
					v.origin.DrawSphere(0.2f, v.color);
					raylib::Vector3 ref = (fabsf(dir.y) < 0.9f)
						? raylib::Vector3{ 0, 1, 0 } : raylib::Vector3{ 1, 0, 0 };
					raylib::Vector3 u = dir.CrossProduct(ref).Normalize().Scale(0.5f);
					raylib::Vector3 w = dir.CrossProduct(u).Normalize().Scale(0.5f);
					raylib::Vector3(v.origin).DrawLine3D(end, v.color);
					for (auto off : { u, u.Scale(-1.0f), w, w.Scale(-1.0f) }) {
						raylib::Vector3 o = raylib::Vector3(v.origin).Add(off);
						o.DrawLine3D(o.Add(dir.Scale(v.length)), v.color);
					}
				}
			}, cmd);
		}
	}
	void CommandDebugQueue::Clear()
	{
		m_commands.clear(); 
	}
}