#include "CommandQueue.hpp"
#include "engine/Material.hpp"
#include "engine/AssetManager.hpp"
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
		//const Command* cmds = m_commands.data();
		std::sort(m_order.begin(), m_order.end(),
			[cmds = m_commands.data()](const uint32_t& ia, const uint32_t& ib) {
				const Command& a = cmds[ia];
				const Command& b = cmds[ib];
				uint32_t sa = a.material ? a.material->GetShaderId() : 0;
				uint32_t sb = b.material ? b.material->GetShaderId() : 0;
				if (sa != sb) return sa < sb;
				if (a.material != b.material) return a.material < b.material;
				return a.mesh < b.mesh; 
			});
	}

	void CommandQueue::BuildBatches()
	{
		m_batchCount = 0;
		Batch* current = nullptr;
		const Command* cmds = m_commands.data();
		for (uint32_t idx : m_order) {
			const Command& cmd = cmds[idx];
			if (cmd.isCulled || !cmd.mesh || !cmd.material) {
				continue;
			}
			if (!current || current->mesh != cmd.mesh || current->material != cmd.material)
			{
				if (m_batchCount == m_batches.size()) {
					m_batches.emplace_back();
				}
				current = &m_batches[m_batchCount++];
				current->mesh = cmd.mesh;
				current->material = cmd.material;
				current->transforms.clear(); 
			}
			current->transforms.emplace_back(cmd.worldMatrix);
		}
	}

	void CommandQueue::Execute(AssetManager& assets, RenderStats& stats)
	{
		stats.materialCount = assets.materialCount();
		constexpr size_t kInstanceThreshold = 4;
		for (size_t b = 0; b < m_batchCount; ++b) {
			const Batch& batch = m_batches[b];
			if (!assets.IsValidShader(batch.material->GetShaderId())) {
				continue;
			}
			const size_t count = batch.transforms.size();
			uint32_t baseShaderId = batch.material->GetShaderId();
			uint32_t instShaderId = assets.GetInstancedShaderId(baseShaderId);

			if (count >= kInstanceThreshold && instShaderId != AssetManager::Invalid) {
				raylib::Shader& instShader = assets.GetShader(instShaderId);
				raylib::Material& rlMat = batch.material->Apply(instShader);
				// Instanced draw via raylib-cpp's Mesh::Draw(material, transforms, n)
				// (wraps ::DrawMeshInstanced).
				batch.mesh->Draw(rlMat, (const Matrix*)batch.transforms.data(), (int)count);
				stats.drawCalls++;
				stats.stageCount++;
				stats.triangles += (uint32_t)batch.mesh->GetTriangleCount() * (uint32_t)count;
				stats.vertices += (uint32_t)batch.mesh->GetVertexCount() * (uint32_t)count;
			}
			else {
				raylib::Shader& shader = assets.GetShader(baseShaderId);
				raylib::Material& rlMat = batch.material->Apply(shader);
				stats.stageCount++;
				for (const raylib::Matrix& m : batch.transforms) {
					batch.mesh->Draw(rlMat, m);
					stats.drawCalls++;
					stats.triangles += (uint32_t)batch.mesh->GetTriangleCount();
					stats.vertices += (uint32_t)batch.mesh->GetVertexCount();
				}
			}
		}
	}
}