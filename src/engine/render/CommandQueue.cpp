#include "CommandQueue.hpp"
#include "engine/Material.hpp"
#include "engine/AssetManager.hpp"
#include <algorithm>
namespace Long {
	void CommandQueue::Submit(const Command& cmd)
	{
		m_commands.emplace_back(cmd);
	}
	void CommandQueue::Clear()
	{
		m_commands.clear();
	}
	const std::vector<Command>& CommandQueue::GetCommands()
	{
		return m_commands;
	}

	void CommandQueue::Sort()
	{
		std::sort(m_commands.begin(), m_commands.end(),
			[](const Command& a, const Command& b) {
				uint32_t sa = a.material ? a.material->GetShaderId() : 0;
				uint32_t sb = b.material ? b.material->GetShaderId() : 0;
				if (sa != sb) return sa < sb;
				return a.material < b.material; // group same material together
			});
	}

	void CommandQueue::Execute(AssetManager& assets, RenderStats& stats)
	{
		BaseMaterial* lastMaterial = nullptr;
		raylib::Material* rlMat = nullptr; // the applied raylib material to draw with

		for (const Command& cmd : m_commands) {
			if (cmd.isCulled || !cmd.mesh || !cmd.material) {
				continue;
			}
			if (!assets.IsValidShader(cmd.material->GetShaderId())) {
				continue;
			}
			if (cmd.material != lastMaterial) {
				raylib::Shader& shader = assets.GetShader(cmd.material->GetShaderId());
				rlMat = &cmd.material->Apply(shader); 
				lastMaterial = cmd.material;
			}
			cmd.mesh->Draw(*rlMat, cmd.worldMatrix);
			stats.drawCalls++;
			stats.triangles += (uint32_t)cmd.mesh->GetTriangleCount();
			stats.vertices  += (uint32_t)cmd.mesh->GetVertexCount();
		}
	}
}