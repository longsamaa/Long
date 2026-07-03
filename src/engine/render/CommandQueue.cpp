#include "CommandQueue.hpp"
#include "engine/Material.hpp"
#include "engine/AssetManager.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <algorithm>
#include <cstring>
#include <system/LightSystem.hpp>

// Not exposed by raylib's public headers -- rmodels.c defines it privately the
// same way (see rmodels.c: "Maximum number of maps supported").
#ifndef MAX_MATERIAL_MAPS
	#define MAX_MATERIAL_MAPS 12
#endif

namespace Long {
	// ---------------------------------------------------------------------
	// Custom draw path (replaces raylib DrawMesh / DrawMeshInstanced).
	//
	// raylib's DrawMesh redoes EVERYTHING per call: glUseProgram, upload
	// view/projection, material colors, bind textures + VAO, draw, then
	// rlDisableShader(). For a batch of N transforms that is N full state
	// setups where only the model matrix actually changes. Here:
	//   - shader + view/proj:      once per GL program (batches are shader-sorted)
	//   - material colors/textures: once per batch
	//   - per draw:                 only the model-dependent matrices
	// DrawMeshInstanced also allocates + frees an instance VBO every call; we
	// keep one persistent VBO and just update it (UploadInstanceTransforms).
	// Limitations: no GPU skinning / stereo rendering (the editor uses neither).
	// ---------------------------------------------------------------------

	// Upload colDiffuse / colSpecular from the raylib material (mirrors DrawMesh).
	static void UploadMaterialColors(const ::Material& mat) {
		if (mat.shader.locs[SHADER_LOC_COLOR_DIFFUSE] != -1) {
			float values[4] = {
				(float)mat.maps[MATERIAL_MAP_DIFFUSE].color.r / 255.0f,
				(float)mat.maps[MATERIAL_MAP_DIFFUSE].color.g / 255.0f,
				(float)mat.maps[MATERIAL_MAP_DIFFUSE].color.b / 255.0f,
				(float)mat.maps[MATERIAL_MAP_DIFFUSE].color.a / 255.0f
			};
			rlSetUniform(mat.shader.locs[SHADER_LOC_COLOR_DIFFUSE], values, SHADER_UNIFORM_VEC4, 1);
		}
		if (mat.shader.locs[SHADER_LOC_COLOR_SPECULAR] != -1) {
			float values[4] = {
				(float)mat.maps[MATERIAL_MAP_SPECULAR].color.r / 255.0f,
				(float)mat.maps[MATERIAL_MAP_SPECULAR].color.g / 255.0f,
				(float)mat.maps[MATERIAL_MAP_SPECULAR].color.b / 255.0f,
				(float)mat.maps[MATERIAL_MAP_SPECULAR].color.a / 255.0f
			};
			rlSetUniform(mat.shader.locs[SHADER_LOC_COLOR_SPECULAR], values, SHADER_UNIFORM_VEC4, 1);
		}
	}

	// Upload the scene light array to the CURRENTLY BOUND shader program.
	// Called once per program change in Execute (batches are shader-sorted), so
	// the cost is a handful of glUniform calls per shader per frame -- never per
	// draw. Uniform names must match default.frag. Shaders that don't declare
	// them (wireframe, emissive, ...) resolve to location -1 and are skipped.
	// SoA layout: one rlSetUniform per attribute uploads the whole array.
	static void BindLights(const raylib::Shader& shader, const SceneLights& lights) {
		const int count = (int)lights.size;
		int loc = rlGetLocationUniform(shader.id, "u_lightCount");
		if (loc == -1) {
			return; // shader is unlit -> skip the rest
		}
		rlSetUniform(loc, &count, SHADER_UNIFORM_INT, 1);
		if (count <= 0) {
			return;
		}

		float pos[SceneLights::kMaxLights * 3];
		float dir[SceneLights::kMaxLights * 3];
		float col[SceneLights::kMaxLights * 4];
		float inten[SceneLights::kMaxLights];
		int   type[SceneLights::kMaxLights];
		for (int i = 0; i < count; ++i) {
			const LightParameter& l = lights.lights[i];
			pos[i * 3 + 0] = l.position.x;
			pos[i * 3 + 1] = l.position.y;
			pos[i * 3 + 2] = l.position.z;
			dir[i * 3 + 0] = l.direction.x;
			dir[i * 3 + 1] = l.direction.y;
			dir[i * 3 + 2] = l.direction.z;
			col[i * 4 + 0] = l.color.x;
			col[i * 4 + 1] = l.color.y;
			col[i * 4 + 2] = l.color.z;
			col[i * 4 + 3] = l.color.w;
			inten[i] = l.intensity;
			type[i] = (int)l.type;
		}
		// Querying an array uniform by name returns the location of element [0].
		loc = rlGetLocationUniform(shader.id, "u_lightPos");
		if (loc != -1) rlSetUniform(loc, pos, SHADER_UNIFORM_VEC3, count);
		loc = rlGetLocationUniform(shader.id, "u_lightDir");
		if (loc != -1) rlSetUniform(loc, dir, SHADER_UNIFORM_VEC3, count);
		loc = rlGetLocationUniform(shader.id, "u_lightColor");
		if (loc != -1) rlSetUniform(loc, col, SHADER_UNIFORM_VEC4, count);
		loc = rlGetLocationUniform(shader.id, "u_lightIntensity");
		if (loc != -1) rlSetUniform(loc, inten, SHADER_UNIFORM_FLOAT, count);
		loc = rlGetLocationUniform(shader.id, "u_lightType");
		if (loc != -1) rlSetUniform(loc, type, SHADER_UNIFORM_INT, count);
	}

	// Bind/unbind the material's texture maps to their slots (mirrors DrawMesh).
	static void BindMaterialMaps(const ::Material& mat) {
		for (int i = 0; i < MAX_MATERIAL_MAPS; i++) {
			if (mat.maps[i].texture.id > 0) {
				rlActiveTextureSlot(i);
				if ((i == MATERIAL_MAP_IRRADIANCE) ||
					(i == MATERIAL_MAP_PREFILTER) ||
					(i == MATERIAL_MAP_CUBEMAP)) rlEnableTextureCubemap(mat.maps[i].texture.id);
				else rlEnableTexture(mat.maps[i].texture.id);
				rlSetUniform(mat.shader.locs[SHADER_LOC_MAP_DIFFUSE + i], &i, SHADER_UNIFORM_INT, 1);
			}
		}
	}
	static void UnbindMaterialMaps(const ::Material& mat) {
		for (int i = 0; i < MAX_MATERIAL_MAPS; i++) {
			if (mat.maps[i].texture.id > 0) {
				rlActiveTextureSlot(i);
				if ((i == MATERIAL_MAP_IRRADIANCE) ||
					(i == MATERIAL_MAP_PREFILTER) ||
					(i == MATERIAL_MAP_CUBEMAP)) rlDisableTextureCubemap();
				else rlDisableTexture();
			}
		}
	}

	// Wire the instance-transform VBO into the currently bound VAO as 4 vec4
	// attributes with divisor 1 (mirrors DrawMeshInstanced's attribute setup).
	static void SetupInstanceAttributes(const ::Shader& shader, unsigned int vbo) {
		int loc = shader.locs[SHADER_LOC_VERTEX_INSTANCETRANSFORM];
		if (loc == -1) {
			return;
		}
		rlEnableVertexBuffer(vbo);
		for (int i = 0; i < 4; i++) {
			rlEnableVertexAttribute(loc + i);
			rlSetVertexAttribute(loc + i, 4, RL_FLOAT, 0,
				(int)sizeof(::Matrix), i * (int)sizeof(::Vector4));
			rlSetVertexAttributeDivisor(loc + i, 1);
		}
		rlDisableVertexBuffer();
	}

	CommandQueue::~CommandQueue()
	{
		if (m_instanceVbo != 0) {
			rlUnloadVertexBuffer(m_instanceVbo);
		}
	}

	void CommandQueue::UploadInstanceTransforms(const std::vector<raylib::Matrix>& transforms)
	{
		const size_t count = transforms.size();
		m_instanceStaging.resize(count * 16);
		for (size_t i = 0; i < count; ++i) {
			float16 f = MatrixToFloatV(transforms[i]);
			std::memcpy(&m_instanceStaging[i * 16], f.v, sizeof(f.v));
		}
		const int bytes = (int)(count * 16 * sizeof(float));
		if (m_instanceVbo == 0 || count > m_instanceCapacity) {
			if (m_instanceVbo != 0) {
				rlUnloadVertexBuffer(m_instanceVbo);
			}
			m_instanceVbo = rlLoadVertexBuffer(m_instanceStaging.data(), bytes, true); // dynamic
			m_instanceCapacity = count;
		}
		else {
			rlUpdateVertexBuffer(m_instanceVbo, m_instanceStaging.data(), bytes, 0);
		}
	}

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

	void CommandQueue::Execute(AssetManager& assets, RenderStats& stats, const SceneLights* lights)
	{
		stats.materialCount = assets.materialCount();
		constexpr size_t kInstanceThreshold = 4;

		// Camera matrices are fixed for the whole queue (set by BeginMode3D):
		// read them once instead of per draw like DrawMesh does.
		const ::Matrix matView = rlGetMatrixModelview();
		const ::Matrix matProjection = rlGetMatrixProjection();
		const ::Matrix matStack = rlGetMatrixTransform(); // rlPushMatrix stack, normally identity

		uint32_t activeProgram = 0; // GL program currently bound (0 = none yet)

		for (size_t b = 0; b < m_batchCount; ++b) {
			const Batch& batch = m_batches[b];
			uint32_t baseShaderId = batch.material->GetShaderId();
			if (!assets.IsValidShader(baseShaderId)) {
				continue;
			}
			const size_t count = batch.transforms.size();
			uint32_t instShaderId = assets.GetInstancedShaderId(baseShaderId);
			const bool instanced =
				(count >= kInstanceThreshold && instShaderId != AssetManager::Invalid);

			raylib::Shader& shader = assets.GetShader(instanced ? instShaderId : baseShaderId);

			// Custom per-material uniforms (u_baseColor etc). SetShaderValue inside
			// binds the program itself, so the program is active after this.
			raylib::Material& rlMat = batch.material->Apply(shader);
			const ::Shader sh = rlMat.shader;

			// Batches are shader-sorted: only rebind + re-upload camera matrices
			// when the GL program actually changes.
			if (sh.id != activeProgram) {
				rlEnableShader(sh.id);
				activeProgram = sh.id;
				stats.shaderBinds++;
				if (sh.locs[SHADER_LOC_MATRIX_VIEW] != -1) {
					rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_VIEW], matView);
				}
				if (sh.locs[SHADER_LOC_MATRIX_PROJECTION] != -1) {
					rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_PROJECTION], matProjection);
				}
				// Global per-shader uniforms: the scene light array.
				if (lights != nullptr) {
					BindLights(shader, *lights);
				}
			}

			// Per-batch material state (colors + texture maps).
			UploadMaterialColors(rlMat);
			BindMaterialMaps(rlMat);

			raylib::Mesh& mesh = *batch.mesh;
			if (!rlEnableVertexArray(mesh.vaoId)) {
				// No VAO (unexpected on GL 4.3) -> raylib's own slow path.
				UnbindMaterialMaps(rlMat);
				stats.stageCount++;
				for (const raylib::Matrix& m : batch.transforms) {
					mesh.Draw(rlMat, m);
					stats.drawCalls++;
					stats.triangles += (uint32_t)mesh.triangleCount;
					stats.vertices += (uint32_t)mesh.vertexCount;
				}
				activeProgram = 0; // DrawMesh disables the shader on exit
				continue;
			}

			if (instanced) {
				// Model matrix comes from the per-instance vertex attribute; the
				// MVP uniform only carries stack*view*proj (mirrors DrawMeshInstanced).
				UploadInstanceTransforms(batch.transforms);
				SetupInstanceAttributes(sh, m_instanceVbo);
				if (sh.locs[SHADER_LOC_MATRIX_NORMAL] != -1) {
					rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_NORMAL], MatrixIdentity());
				}
				rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP],
					MatrixMultiply(MatrixMultiply(matStack, matView), matProjection));
				if (mesh.indices != NULL) {
					rlDrawVertexArrayElementsInstanced(0, mesh.triangleCount * 3, 0, (int)count);
				}
				else {
					rlDrawVertexArrayInstanced(0, mesh.vertexCount, (int)count);
				}
				stats.drawCalls++;
				stats.triangles += (uint32_t)mesh.triangleCount * (uint32_t)count;
				stats.vertices += (uint32_t)mesh.vertexCount * (uint32_t)count;
			}
			else {
				// Only the model-dependent matrices change per draw.
				for (const raylib::Matrix& t : batch.transforms) {
					const ::Matrix matModel = MatrixMultiply(t, matStack);
					if (sh.locs[SHADER_LOC_MATRIX_MODEL] != -1) {
						rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MODEL], matModel);
					}
					if (sh.locs[SHADER_LOC_MATRIX_NORMAL] != -1) {
						rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_NORMAL],
							MatrixTranspose(MatrixInvert(matModel)));
					}
					rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP],
						MatrixMultiply(MatrixMultiply(matModel, matView), matProjection));
					if (mesh.indices != NULL) {
						rlDrawVertexArrayElements(0, mesh.triangleCount * 3, 0);
					}
					else {
						rlDrawVertexArray(0, mesh.vertexCount);
					}
					stats.drawCalls++;
					stats.triangles += (uint32_t)mesh.triangleCount;
					stats.vertices += (uint32_t)mesh.vertexCount;
				}
			}

			rlDisableVertexArray();
			UnbindMaterialMaps(rlMat);
			stats.stageCount++;
		}

		// One unbind for the whole queue instead of one per draw.
		rlDisableShader();
	}
}