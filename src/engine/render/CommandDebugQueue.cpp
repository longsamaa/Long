#include "CommandDebugQueue.hpp"
#include <raylib-cpp.hpp>
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
				}, cmd);
		}
	}
	void CommandDebugQueue::Clear()
	{
		m_commands.clear(); 
	}
}