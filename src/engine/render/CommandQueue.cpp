#include "CommandQueue.hpp"
#include "engine/Material.hpp"
#include "engine/AssetManager.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <algorithm>
#include <cstring>
#include <cstdio>   // snprintf
#include <system/LightSystem.hpp>

#ifndef MAX_MATERIAL_MAPS
#define MAX_MATERIAL_MAPS 12
#endif

namespace Long {
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

	static void BindLights(const raylib::Shader& shader, const SceneLights& lights) {
		const int count = (int)lights.size;
		int loc = rlGetLocationUniform(shader.id, "u_lightCount");
		if (loc == -1) {
			return; // shader is unlit -> skip the rest
		}
		rlSetUniform(loc, &count, SHADER_UNIFORM_INT, 1);

		// Hemisphere ambient (PBR shaders; others resolve to -1 and skip).
		loc = rlGetLocationUniform(shader.id, "u_ambientSky");
		if (loc != -1) {
			float v[3]{ lights.ambientSky.x, lights.ambientSky.y, lights.ambientSky.z };
			rlSetUniform(loc, v, SHADER_UNIFORM_VEC3, 1);
		}
		loc = rlGetLocationUniform(shader.id, "u_ambientGround");
		if (loc != -1) {
			float v[3]{ lights.ambientGround.x, lights.ambientGround.y, lights.ambientGround.z };
			rlSetUniform(loc, v, SHADER_UNIFORM_VEC3, 1);
		}
		loc = rlGetLocationUniform(shader.id, "u_ambientIntensity");
		if (loc != -1) {
			rlSetUniform(loc, &lights.ambientIntensity, SHADER_UNIFORM_FLOAT, 1);
		}

		if (count <= 0) {
			return;
		}

		char name[64];
		for (int i = 0; i < count; ++i) {
			const LightParameter& l = lights.lights[i];

			snprintf(name, sizeof(name), "u_lights[%d].position", i);
			loc = rlGetLocationUniform(shader.id, name);
			if (loc != -1) { float v[3]{ l.position.x, l.position.y, l.position.z }; rlSetUniform(loc, v, SHADER_UNIFORM_VEC3, 1); }

			snprintf(name, sizeof(name), "u_lights[%d].direction", i);
			loc = rlGetLocationUniform(shader.id, name);
			if (loc != -1) { float v[3]{ l.direction.x, l.direction.y, l.direction.z }; rlSetUniform(loc, v, SHADER_UNIFORM_VEC3, 1); }

			snprintf(name, sizeof(name), "u_lights[%d].color", i);
			loc = rlGetLocationUniform(shader.id, name);
			if (loc != -1) { float v[4]{ l.color.x, l.color.y, l.color.z, l.color.w }; rlSetUniform(loc, v, SHADER_UNIFORM_VEC4, 1); }

			snprintf(name, sizeof(name), "u_lights[%d].intensity", i);
			loc = rlGetLocationUniform(shader.id, name);
			if (loc != -1) { float v = l.intensity; rlSetUniform(loc, &v, SHADER_UNIFORM_FLOAT, 1); }

			snprintf(name, sizeof(name), "u_lights[%d].type", i);
			loc = rlGetLocationUniform(shader.id, name);
			if (loc != -1) { int v = (int)l.type; rlSetUniform(loc, &v, SHADER_UNIFORM_INT, 1); }

			snprintf(name, sizeof(name), "u_lights[%d].innerCos", i);
			loc = rlGetLocationUniform(shader.id, name);
			if (loc != -1) { float v = l.innerCos; rlSetUniform(loc, &v, SHADER_UNIFORM_FLOAT, 1); }

			snprintf(name, sizeof(name), "u_lights[%d].outerCos", i);
			loc = rlGetLocationUniform(shader.id, name);
			if (loc != -1) { float v = l.outerCos; rlSetUniform(loc, &v, SHADER_UNIFORM_FLOAT, 1); }

			snprintf(name, sizeof(name), "u_lights[%d].range", i);
			loc = rlGetLocationUniform(shader.id, name);
			if (loc != -1) { float v = l.range; rlSetUniform(loc, &v, SHADER_UNIFORM_FLOAT, 1); }

			snprintf(name, sizeof(name), "u_lights[%d].shadowIndex", i);
			loc = rlGetLocationUniform(shader.id, name);
			if (loc != -1) { int v = l.shadowIndex; rlSetUniform(loc, &v, SHADER_UNIFORM_INT, 1); }
		}
	}

	// Bind every rendered shadow map + its light-space matrix to the current
	// shader (arrays u_shadowMaps[] / u_lightViewProj[]; each light carries its
	// shadowIndex into them). High texture slots (10+) stay clear of material
	// maps (0..MAX_MATERIAL_MAPS). Called once per program change.
	static constexpr int kShadowTexSlot0 = 10;
	static void BindShadow(const raylib::Shader& shader, const SceneLights& lights) {
		int countLoc = rlGetLocationUniform(shader.id, "u_shadowCount");
		if (countLoc == -1) {
			return; // shader doesn't support shadows
		}
		int count = (int)lights.shadowCount;
		rlSetUniform(countLoc, &count, SHADER_UNIFORM_INT, 1);

		char name[64];
		for (int k = 0; k < count; ++k) {
			const ShadowCaster& sc = lights.shadows[k];
			snprintf(name, sizeof(name), "u_lightViewProj[%d]", k);
			int mvpLoc = rlGetLocationUniform(shader.id, name);
			if (mvpLoc != -1) {
				rlSetUniformMatrix(mvpLoc, sc.lightViewProj);
			}
			snprintf(name, sizeof(name), "u_shadowMaps[%d]", k);
			int mapLoc = rlGetLocationUniform(shader.id, name);
			if (mapLoc != -1 && sc.depthTexId != 0) {
				int slot = kShadowTexSlot0 + k;
				rlActiveTextureSlot(slot);
				rlEnableTexture(sc.depthTexId);
				rlSetUniform(mapLoc, &slot, SHADER_UNIFORM_INT, 1);
			}
		}
	}

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
		float* dst = m_instanceStaging.data();
		for (size_t i = 0; i < count; ++i, dst += 16) {
			const ::Matrix& m = transforms[i];
			dst[0] = m.m0;  dst[1] = m.m1;  dst[2] = m.m2;  dst[3] = m.m3;
			dst[4] = m.m4;  dst[5] = m.m5;  dst[6] = m.m6;  dst[7] = m.m7;
			dst[8] = m.m8;  dst[9] = m.m9;  dst[10] = m.m10; dst[11] = m.m11;
			dst[12] = m.m12; dst[13] = m.m13; dst[14] = m.m14; dst[15] = m.m15;
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
		const Command* cmds = m_commands.data();
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

		const raylib::Matrix matView(rlGetMatrixModelview());
		const raylib::Matrix matProjection(rlGetMatrixProjection());
		const raylib::Matrix matStack(rlGetMatrixTransform()); // rlPushMatrix stack, normally identity
	
		// World-space camera position = translation of the inverse view matrix.
		// Fixed for the whole queue; specular in the shader needs it (u_viewPos).
		const raylib::Matrix invView = raylib::Matrix(MatrixInvert(matView));
		const float camPos[3] = { invView.m12, invView.m13, invView.m14 };

		const raylib::Matrix viewProj = matView.Multiply(matProjection);
		const raylib::Matrix viewProjStack = matStack.Multiply(matView).Multiply(matProjection); 
		//MatrixMultiply(MatrixMultiply(matStack, matView), matProjection)


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
			raylib::Material& rlMat = batch.material->Apply(shader);
			const ::Shader sh = rlMat.shader;

			if (sh.id != activeProgram) {
				rlEnableShader(sh.id);
				stats.stageCount++; 
				activeProgram = sh.id;
				stats.shaderBinds++;
				if (sh.locs[SHADER_LOC_MATRIX_VIEW] != -1) {
					rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_VIEW], matView);
				}
				if (sh.locs[SHADER_LOC_MATRIX_PROJECTION] != -1) {
					rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_PROJECTION], matProjection);
				}
				// Camera position for specular. u_* so raylib never overwrites it;
				// unlit shaders resolve it to -1 and skip.
				int viewPosLoc = rlGetLocationUniform(sh.id, "u_viewPos");
				if (viewPosLoc != -1) {
					rlSetUniform(viewPosLoc, camPos, SHADER_UNIFORM_VEC3, 1);
				}
				// Global per-shader uniforms: the scene light array + shadow map.
				if (lights != nullptr) {
					BindLights(shader, *lights);
					BindShadow(shader, *lights);
				}
			}

			UploadMaterialColors(rlMat);
			BindMaterialMaps(rlMat);

			raylib::Mesh& mesh = *batch.mesh;
			if (!rlEnableVertexArray(mesh.vaoId)) {
				// No VAO (unexpected on GL 4.3) -> raylib's own slow path.
				UnbindMaterialMaps(rlMat);
				//stats.stageCount++;
				for (const raylib::Matrix& m : batch.transforms) {
					mesh.Draw(rlMat, m);
					stats.drawCalls++;
					stats.triangles += (uint32_t)mesh.triangleCount;
					stats.vertices += (uint32_t)mesh.vertexCount;
				}
				activeProgram = 0;
				continue;
			}
			if (instanced) {
				UploadInstanceTransforms(batch.transforms);
				SetupInstanceAttributes(sh, m_instanceVbo);
				if (sh.locs[SHADER_LOC_MATRIX_NORMAL] != -1) {
					rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_NORMAL], MatrixIdentity());
				}
				//rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP],
				//	MatrixMultiply(MatrixMultiply(matStack, matView), matProjection));
				rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP], viewProjStack);
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
					const raylib::Matrix matModel = t.Multiply(matStack);
					if (sh.locs[SHADER_LOC_MATRIX_MODEL] != -1) {
						rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MODEL], matModel);
					}
					if (sh.locs[SHADER_LOC_MATRIX_NORMAL] != -1) {
						rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_NORMAL],
							MatrixTranspose(MatrixInvert(matModel)));
					}
					//rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP], matModel.Multiply(matView).Multiply(matProjection));
					rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP], matModel.Multiply(viewProj));
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
			//stats.stageCount++;
		}
		rlDisableShader();
	}

	void CommandQueue::ExecuteDepth(AssetManager& assets, const raylib::Matrix& lightViewProj,
		uint32_t depthShaderId)
	{
		if (!assets.IsValidShader(depthShaderId)) {
			return;
		}
		constexpr size_t kInstanceThreshold = 4;
		const raylib::Matrix lvp = lightViewProj;
		const uint32_t instDepthId = assets.GetInstancedShaderId(depthShaderId);
		uint32_t activeProgram = 0;
		for (size_t b = 0; b < m_batchCount; ++b) {
			const Batch& batch = m_batches[b];
			if (!batch.mesh || !batch.material || !batch.material->CastsShadow()) {
				continue; // non-casters don't write to the shadow map
			}
			const size_t count = batch.transforms.size();
			const bool instanced =
				(count >= kInstanceThreshold && instDepthId != AssetManager::Invalid);

			const ::Shader sh = assets.GetShader(instanced ? instDepthId : depthShaderId);
			if (sh.id != activeProgram) {
				rlEnableShader(sh.id);
				activeProgram = sh.id;
			}

			raylib::Mesh& mesh = *batch.mesh;
			if (!rlEnableVertexArray(mesh.vaoId)) {
				continue; // depth pass skips meshes without a VAO
			}

			if (instanced) {
				// mvp = lightViewProj only; per-instance model from the attribute.
				UploadInstanceTransforms(batch.transforms);
				SetupInstanceAttributes(sh, m_instanceVbo);
				rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP], lvp);
				if (mesh.indices != NULL) {
					rlDrawVertexArrayElementsInstanced(0, mesh.triangleCount * 3, 0, (int)count);
				}
				else {
					rlDrawVertexArrayInstanced(0, mesh.vertexCount, (int)count);
				}
			}
			else {
				for (const raylib::Matrix& t : batch.transforms) {
					// mvp already folds the model matrix for the single-draw path.
					rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP],
						MatrixMultiply(t, lvp));
					if (mesh.indices != NULL) {
						rlDrawVertexArrayElements(0, mesh.triangleCount * 3, 0);
					}
					else {
						rlDrawVertexArray(0, mesh.vertexCount);
					}
				}
			}
			rlDisableVertexArray();
		}
		rlDisableShader();
	}
}