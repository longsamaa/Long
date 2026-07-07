#include "GLRenderer.hpp"
#include "CommandQueue.hpp"   // Batch
#include "engine/Material.hpp"
#include "engine/AssetManager.hpp"
#include "rlgl.h"
#include "raymath.h"
#include <cstdio>   // snprintf
#include <unordered_map>
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
	static constexpr int kShadowTexSlot0 = 10;                              // 2D maps: slots 10..
	static constexpr int kCubeTexSlot0 = kShadowTexSlot0 + SceneLights::kMaxShadows; // cubes after
	struct LightLocs {
		int count = -1;                 // -1 here also means "shader is unlit" (early out)
		int ambientSky = -1, ambientGround = -1, ambientIntensity = -1;
		struct Per {
			int position = -1, direction = -1, color = -1, intensity = -1;
			int type = -1, innerCos = -1, outerCos = -1, range = -1, shadowIndex = -1;
			int cubeShadowIndex = -1;
		} light[SceneLights::kMaxLights];
		// shadows
		int shadowCount = -1;
		int lightViewProj[SceneLights::kMaxShadows] = {};
		int shadowMaps[SceneLights::kMaxShadows] = {};
		// point-light cube shadows
		int cubeShadowCount = -1;
		int pointShadowMaps[SceneLights::kMaxCubeShadows] = {};
		int pointLightPos[SceneLights::kMaxCubeShadows] = {};
		int pointLightRange[SceneLights::kMaxCubeShadows] = {};
		int viewPos = -1;               // camera pos (specular); resolved here too
	};

	// One entry per shader program id; filled lazily on first use.
	static std::unordered_map<unsigned int, LightLocs> s_lightLocs;

	static const LightLocs& GetLightLocs(unsigned int shaderId) {
		auto it = s_lightLocs.find(shaderId);
		if (it != s_lightLocs.end()) {
			return it->second;
		}
		LightLocs L;
		char name[64];
		L.count = rlGetLocationUniform(shaderId, "u_lightCount");
		L.ambientSky = rlGetLocationUniform(shaderId, "u_ambientSky");
		L.ambientGround = rlGetLocationUniform(shaderId, "u_ambientGround");
		L.ambientIntensity = rlGetLocationUniform(shaderId, "u_ambientIntensity");
		L.viewPos = rlGetLocationUniform(shaderId, "u_viewPos");
		for (int i = 0; i < SceneLights::kMaxLights; ++i) {
			auto& p = L.light[i];
			snprintf(name, sizeof(name), "u_lights[%d].position", i);    p.position = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_lights[%d].direction", i);   p.direction = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_lights[%d].color", i);       p.color = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_lights[%d].intensity", i);   p.intensity = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_lights[%d].type", i);        p.type = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_lights[%d].innerCos", i);    p.innerCos = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_lights[%d].outerCos", i);    p.outerCos = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_lights[%d].range", i);       p.range = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_lights[%d].shadowIndex", i); p.shadowIndex = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_lights[%d].cubeShadowIndex", i); p.cubeShadowIndex = rlGetLocationUniform(shaderId, name);
		}
		L.shadowCount = rlGetLocationUniform(shaderId, "u_shadowCount");
		for (int k = 0; k < SceneLights::kMaxShadows; ++k) {
			snprintf(name, sizeof(name), "u_lightViewProj[%d]", k); L.lightViewProj[k] = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_shadowMaps[%d]", k);    L.shadowMaps[k] = rlGetLocationUniform(shaderId, name);
		}
		L.cubeShadowCount = rlGetLocationUniform(shaderId, "u_cubeShadowCount");
		for (int k = 0; k < SceneLights::kMaxCubeShadows; ++k) {
			snprintf(name, sizeof(name), "u_pointShadowMaps[%d]", k);  L.pointShadowMaps[k] = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_pointLightPos[%d]", k);    L.pointLightPos[k] = rlGetLocationUniform(shaderId, name);
			snprintf(name, sizeof(name), "u_pointLightRange[%d]", k);  L.pointLightRange[k] = rlGetLocationUniform(shaderId, name);
		}
		return s_lightLocs.emplace(shaderId, L).first->second;
	}

	static void BindLights(const raylib::Shader& shader, const SceneLights& lights) {
		const LightLocs& L = GetLightLocs(shader.id);
		if (L.count == -1) {
			return; // shader is unlit -> nothing to bind
		}
		const int count = (int)lights.size;
		rlSetUniform(L.count, &count, SHADER_UNIFORM_INT, 1);

		// Hemisphere ambient (PBR shaders; others cached as -1 and skip).
		if (L.ambientSky != -1) {
			float v[3]{ lights.ambientSky.x, lights.ambientSky.y, lights.ambientSky.z };
			rlSetUniform(L.ambientSky, v, SHADER_UNIFORM_VEC3, 1);
		}
		if (L.ambientGround != -1) {
			float v[3]{ lights.ambientGround.x, lights.ambientGround.y, lights.ambientGround.z };
			rlSetUniform(L.ambientGround, v, SHADER_UNIFORM_VEC3, 1);
		}
		if (L.ambientIntensity != -1) {
			rlSetUniform(L.ambientIntensity, &lights.ambientIntensity, SHADER_UNIFORM_FLOAT, 1);
		}

		if (count <= 0) {
			return;
		}
		for (int i = 0; i < count; ++i) {
			const LightParameter& l = lights.lights[i];
			const auto& p = L.light[i];
			if (p.position != -1) { float v[3]{ l.position.x, l.position.y, l.position.z }; rlSetUniform(p.position, v, SHADER_UNIFORM_VEC3, 1); }
			if (p.direction != -1) { float v[3]{ l.direction.x, l.direction.y, l.direction.z }; rlSetUniform(p.direction, v, SHADER_UNIFORM_VEC3, 1); }
			if (p.color != -1) { float v[4]{ l.color.x, l.color.y, l.color.z, l.color.w }; rlSetUniform(p.color, v, SHADER_UNIFORM_VEC4, 1); }
			if (p.intensity != -1) { float v = l.intensity; rlSetUniform(p.intensity, &v, SHADER_UNIFORM_FLOAT, 1); }
			if (p.type != -1) { int v = (int)l.type; rlSetUniform(p.type, &v, SHADER_UNIFORM_INT, 1); }
			if (p.innerCos != -1) { float v = l.innerCos; rlSetUniform(p.innerCos, &v, SHADER_UNIFORM_FLOAT, 1); }
			if (p.outerCos != -1) { float v = l.outerCos; rlSetUniform(p.outerCos, &v, SHADER_UNIFORM_FLOAT, 1); }
			if (p.range != -1) { float v = l.range; rlSetUniform(p.range, &v, SHADER_UNIFORM_FLOAT, 1); }
			if (p.shadowIndex != -1) { int v = l.shadowIndex; rlSetUniform(p.shadowIndex, &v, SHADER_UNIFORM_INT, 1); }
			if (p.cubeShadowIndex != -1) { int v = l.cubeShadowIndex; rlSetUniform(p.cubeShadowIndex, &v, SHADER_UNIFORM_INT, 1); }
		}
	}
	
	static void BindShadow(const raylib::Shader& shader, const SceneLights& lights) {
		const LightLocs& L = GetLightLocs(shader.id);
		if (L.shadowCount == -1) {
			return; // shader doesn't support shadows
		}
		int count = (int)lights.shadowCount;
		rlSetUniform(L.shadowCount, &count, SHADER_UNIFORM_INT, 1);

		for (int k = 0; k < count; ++k) {
			const ShadowCaster& sc = lights.shadows[k];
			if (L.lightViewProj[k] != -1) {
				rlSetUniformMatrix(L.lightViewProj[k], sc.lightViewProj);
			}
			if (L.shadowMaps[k] != -1 && sc.depthTexId != 0) {
				int slot = kShadowTexSlot0 + k;
				rlActiveTextureSlot(slot);
				rlEnableTexture(sc.depthTexId);
				rlSetUniform(L.shadowMaps[k], &slot, SHADER_UNIFORM_INT, 1);
			}
		}

		// Point-light cube shadows (samplerCube). Slots continue after the 2D
		// maps so they never collide. rlEnableTextureCubemap binds to the CUBE
		// target of the active slot.
		if (L.cubeShadowCount != -1) {
			int cubeCount = (int)lights.cubeShadowCount;
			rlSetUniform(L.cubeShadowCount, &cubeCount, SHADER_UNIFORM_INT, 1);
			for (int k = 0; k < cubeCount; ++k) {
				const CubeShadowCaster& cs = lights.cubeShadows[k];
				if (L.pointShadowMaps[k] != -1 && cs.cubeTexId != 0) {
					int slot = kCubeTexSlot0 + k;
					rlActiveTextureSlot(slot);
					rlEnableTextureCubemap(cs.cubeTexId);
					rlSetUniform(L.pointShadowMaps[k], &slot, SHADER_UNIFORM_INT, 1);
				}
				if (L.pointLightPos[k] != -1) {
					float v[3]{ cs.lightPos.x, cs.lightPos.y, cs.lightPos.z };
					rlSetUniform(L.pointLightPos[k], v, SHADER_UNIFORM_VEC3, 1);
				}
				if (L.pointLightRange[k] != -1) {
					float v = cs.range; rlSetUniform(L.pointLightRange[k], &v, SHADER_UNIFORM_FLOAT, 1);
				}
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

	// ------------------------------------------------------------------------

	GLRenderer::~GLRenderer()
	{
		if (m_instanceVbo != 0) {
			rlUnloadVertexBuffer(m_instanceVbo);
		}
	}

	void GLRenderer::UploadInstanceTransforms(const std::vector<raylib::Matrix>& transforms)
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

	void GLRenderer::DrawBatches(const std::vector<Batch>& batches, size_t batchCount,
		AssetManager& assets, RenderStats& stats, const SceneLights* lights)
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

		uint32_t activeProgram = 0; // GL program currently bound (0 = none yet)

		for (size_t b = 0; b < batchCount; ++b) {
			const Batch& batch = batches[b];
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
		}
		rlDisableShader();
	}

	void GLRenderer::DrawDepth(const std::vector<Batch>& batches, size_t batchCount,
		AssetManager& assets, const raylib::Matrix& lightViewProj,
		uint32_t depthShaderId, const float& range, bool linearDistance,
		const raylib::Vector3* lightPos)
	{
		if (!assets.IsValidShader(depthShaderId)) {
			return;
		}
		constexpr size_t kInstanceThreshold = 4;
		const raylib::Matrix lvp = lightViewProj;
		const uint32_t instDepthId = assets.GetInstancedShaderId(depthShaderId);
		// Point-light (cube) path: the shader writes linear distance-to-light, so
		// it needs the light position + range and (non-instanced) the model matrix
		// to reconstruct the world position. Cache their locations per shader.
		const bool pointMode = linearDistance && lightPos != nullptr;
		uint32_t activeProgram = 0;
		for (size_t b = 0; b < batchCount; ++b) {
			const Batch& batch = batches[b];
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
				if (pointMode) {
					int lp = rlGetLocationUniform(sh.id, "u_lightPos");
					if (lp != -1) { float v[3]{ lightPos->x, lightPos->y, lightPos->z }; rlSetUniform(lp, v, SHADER_UNIFORM_VEC3, 1); }
					int lr = rlGetLocationUniform(sh.id, "u_lightRange");
					if (lr != -1) { float v = range; rlSetUniform(lr, &v, SHADER_UNIFORM_FLOAT, 1); }
				}
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
					// Point cube shader also needs the raw model matrix to build
					// the world position (for linear distance).
					if (pointMode && sh.locs[SHADER_LOC_MATRIX_MODEL] != -1) {
						rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MODEL], t);
					}
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
