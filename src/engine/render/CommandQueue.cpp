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
		m_batches.clear();
	}
	const std::vector<Command>& CommandQueue::GetCommands()
	{
		return m_commands;
	}

	void CommandQueue::Sort()
	{
		// Sort so commands that can be batched together end up adjacent. The batch
		// key is (shader, material, mesh): identical material guarantees same
		// shader/texture/uniforms, identical mesh same VBO -> safe to instance.
		std::sort(m_commands.begin(), m_commands.end(),
			[](const Command& a, const Command& b) {
				uint32_t sa = a.material ? a.material->GetShaderId() : 0;
				uint32_t sb = b.material ? b.material->GetShaderId() : 0;
				if (sa != sb) return sa < sb;
				if (a.material != b.material) return a.material < b.material;
				return a.mesh < b.mesh; // group same mesh together for instancing
			});
	}

	void CommandQueue::BuildBatches()
	{
		// Walk the sorted commands and start a new batch whenever (mesh, material)
		// changes. Because Sort() grouped equal keys together, each batch is just a
		// contiguous run -- no hashing needed.
		m_batches.clear();
		Batch* current = nullptr;
		for (const Command& cmd : m_commands) {
			if (cmd.isCulled || !cmd.mesh || !cmd.material) {
				continue;
			}
			if (!current || current->mesh != cmd.mesh || current->material != cmd.material) {
				m_batches.push_back(Batch{ cmd.mesh, cmd.material, {} });
				current = &m_batches.back();
			}
			current->transforms.push_back(cmd.worldMatrix);
		}
	}

	void CommandQueue::Execute(AssetManager& assets, RenderStats& stats)
	{
		stats.materialCount = assets.materialCount();

		// Groups smaller than this aren't worth the instance-buffer setup; draw them
		// one by one instead.
		constexpr size_t kInstanceThreshold = 4;

		for (const Batch& batch : m_batches) {
			if (!assets.IsValidShader(batch.material->GetShaderId())) {
				continue;
			}
			const size_t count = batch.transforms.size();
			uint32_t baseShaderId = batch.material->GetShaderId();
			uint32_t instShaderId = assets.GetInstancedShaderId(baseShaderId);

			if (count >= kInstanceThreshold && instShaderId != AssetManager::Invalid) {
				// One instanced draw for the whole batch.
				raylib::Shader& instShader = assets.GetShader(instShaderId);
				raylib::Material& rlMat = batch.material->Apply(instShader);
				::DrawMeshInstanced(*batch.mesh, rlMat,
					(const Matrix*)batch.transforms.data(), (int)count);
				stats.drawCalls++;
				stats.stageCount++;
				stats.triangles += (uint32_t)batch.mesh->GetTriangleCount() * (uint32_t)count;
				stats.vertices += (uint32_t)batch.mesh->GetVertexCount() * (uint32_t)count;
			}
			else {
				// Small batch (or no instanced variant): regular draws.
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