#include "CommandQueue.hpp"
#include "engine/Material.hpp"
#include <algorithm>

namespace Long {

	void CommandQueue::Submit(const Command& cmd)
	{
		m_commands.emplace_back(cmd);
	}
	void CommandQueue::Reserve(size_t count)
	{
		m_commands.reserve(count);
		m_order.reserve(count);
	}
	void CommandQueue::Clear()
	{
		m_commands.clear();
		m_order.clear();
	}
	const std::vector<Command>& CommandQueue::GetCommands()
	{
		return m_commands;
	}

	void CommandQueue::Sort()
	{
		m_order.resize(m_commands.size());
		for (uint32_t i = 0; i < (uint32_t)m_commands.size(); ++i) {
			m_order[i] = i;
		}
		std::sort(m_order.begin(), m_order.end(),
			[cmds = m_commands.data()](const uint32_t& ia, const uint32_t& ib) {
				const Command& a = cmds[ia];
				const Command& b = cmds[ib];
				uint32_t sa = a.material ? a.material->GetShaderId() : 0;
				uint32_t sb = b.material ? b.material->GetShaderId() : 0;
				if (sa != sb) return sa < sb;
				if (a.material != b.material) return a.material < b.material;
				return a.meshId < b.meshId;
			});
	}

	void CommandQueue::BuildBatches()
	{
		m_batchCount = 0;
		Batch* current = nullptr;
		const Command* cmds = m_commands.data();
		for (uint32_t idx : m_order) {
			const Command& cmd = cmds[idx];
			if (cmd.isCulled || cmd.meshId == UINT32_MAX || !cmd.material) {
				continue;
			}
			if (!current || current->meshId != cmd.meshId || current->material != cmd.material)
			{
				if (m_batchCount == m_batches.size()) {
					m_batches.emplace_back();
				}
				current = &m_batches[m_batchCount++];
				current->meshId = cmd.meshId;
				current->material = cmd.material;
				current->transforms.clear();
			}
			current->transforms.emplace_back(cmd.worldMatrix);
		}
	}
}
