#include "GLRenderer.hpp"
#include "CommandQueue.hpp"        // Batch
#include "CommandDebugQueue.hpp"   // DebugLineVertex
#include "core/Components.hpp"     // Skeleton
#include "engine/Material.hpp"
#include "engine/AssetManager.hpp"
#include "engine/render/RenderState.hpp"
#include "rlgl.h"
#include "raymath.h"
#include "engine/Logger.hpp"
// GL enums/prototypes only -- raylib already loaded the function pointers.
// Needed for glDrawArrays(GL_LINES): rlgl's vertex-array draws are triangles-only.
#include "external/glad.h"
#include <cstdio>   // snprintf
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <system/LightSystem.hpp>

#ifndef MAX_MATERIAL_MAPS
#define MAX_MATERIAL_MAPS 12
#endif

namespace Long {
	static constexpr int kBoneIdsLoc = 7;
	static constexpr int kBoneWeightsLoc = 8;
	// SSBO binding index of the joint-matrices block; must match the
	// `layout(std430, binding = 3)` in every SKINNED shader variant.
	static constexpr int kJointSsboBinding = 3;

	void GLRenderer::BindJointMatrices(const Skeleton& skel) {
		const size_t n = skel.jointMatrices.size();
		if (n == 0) {
			return;
		}
		m_jointStaging.resize(n * 16);
		float* dst = m_jointStaging.data();
		for (const raylib::Matrix& m : skel.jointMatrices) {
			dst[0] = m.m0;  dst[1] = m.m1;  dst[2] = m.m2;  dst[3] = m.m3;
			dst[4] = m.m4;  dst[5] = m.m5;  dst[6] = m.m6;  dst[7] = m.m7;
			dst[8] = m.m8;  dst[9] = m.m9;  dst[10] = m.m10; dst[11] = m.m11;
			dst[12] = m.m12; dst[13] = m.m13; dst[14] = m.m14; dst[15] = m.m15;
			dst += 16;
		}
		const unsigned int bytes = (unsigned int)(m_jointStaging.size() * sizeof(float));
		if (m_jointSsbo == 0 || m_jointSsboCapacity < bytes) {
			if (m_jointSsbo != 0) {
				rlUnloadShaderBuffer(m_jointSsbo);
			}
			m_jointSsbo = rlLoadShaderBuffer(bytes, nullptr, RL_DYNAMIC_COPY);
			m_jointSsboCapacity = bytes;
		}
		rlUpdateShaderBuffer(m_jointSsbo, m_jointStaging.data(), bytes, 0);
		rlBindShaderBuffer(m_jointSsbo, kJointSsboBinding);
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
		if (m_jointSsbo != 0) {
			rlUnloadShaderBuffer(m_jointSsbo);
		}
		if (m_lineVbo != 0) {
			rlUnloadVertexBuffer(m_lineVbo);
		}
		if (m_lineVao != 0) {
			rlUnloadVertexArray(m_lineVao);
		}
		for (GLGpuMesh& m : m_gpuMeshes) {
			if (m.mesh.vaoId > 0) {
				::UnloadMesh(m.mesh);
			}
		}
		if (m_immediateMaterialReady) {
			m_immediateMaterial.shader.id = rlGetShaderIdDefault();
			m_immediateMaterial.shader.locs = nullptr;
			::UnloadMaterial(m_immediateMaterial);
		}
	}

	GLGpuMesh& GLRenderer::UploadGpuMesh(AssetManager& assets, uint32_t meshId)
	{
		static ::Mesh invalidRaylibMesh{};
		static GLGpuMesh invalidGlGpuMesh{ .mesh = invalidRaylibMesh, .skin = std::nullopt };
		if (!assets.IsValidMesh(meshId)) {
			return invalidGlGpuMesh;
		}
		if (m_gpuMeshes.size() < assets.meshCount()) {
			m_gpuMeshes.resize(assets.meshCount(), GLGpuMesh{ .mesh = ::Mesh(),
			.skin = std::nullopt });
		}
		GLGpuMesh& gpu = m_gpuMeshes[meshId];
		if (gpu.mesh.vaoId > 0) {
			return gpu; // already uploaded
		}

		const MeshCPU& cpu = assets.GetMesh(meshId);
		if (!cpu.IsValid()) {
			return invalidGlGpuMesh;
		}
		const int vcount = cpu.VertexCount();
		::Mesh m = uploadRaylibMesh(cpu);
		MemFree(m.vertices);  m.vertices = nullptr;
		MemFree(m.normals);   m.normals = nullptr;
		MemFree(m.texcoords); m.texcoords = nullptr;
		gpu.mesh = m;
		gpu.skin = uploadSkinMesh(m.vaoId, cpu);
		return gpu;
	}

	::Mesh GLRenderer::uploadRaylibMesh(const MeshCPU& cpu)
	{
		::Mesh m{};
		uint32_t vcount = cpu.VertexCount();
		m.vertexCount = vcount;
		m.triangleCount = cpu.TriangleCount();
		m.vertices = (float*)MemAlloc((unsigned int)(vcount * 3 * sizeof(float)));
		m.normals = (float*)MemAlloc((unsigned int)(vcount * 3 * sizeof(float)));
		m.texcoords = (float*)MemAlloc((unsigned int)(vcount * 2 * sizeof(float)));
		for (int i = 0; i < vcount; ++i) {
			const VertexPNT& v = cpu.vertices[(size_t)i];
			m.vertices[i * 3 + 0] = v.px; m.vertices[i * 3 + 1] = v.py; m.vertices[i * 3 + 2] = v.pz;
			m.normals[i * 3 + 0] = v.nx;  m.normals[i * 3 + 1] = v.ny;  m.normals[i * 3 + 2] = v.nz;
			m.texcoords[i * 2 + 0] = v.u; m.texcoords[i * 2 + 1] = v.v;
		}
		if (!cpu.indices.empty()) {
			m.indices = (unsigned short*)MemAlloc(
				(unsigned int)(cpu.indices.size() * sizeof(unsigned short)));
			for (size_t i = 0; i < cpu.indices.size(); ++i) {
				m.indices[i] = (unsigned short)cpu.indices[i];
			}
		}
		::UploadMesh(&m, false);
		return m;
	}

	std::optional<GLSkinMesh> GLRenderer::uploadSkinMesh(uint32_t vaoId, const MeshCPU& cpu)
	{
		if (!cpu.IsSkinned()) return std::nullopt;
		const uint32_t vcount = cpu.VertexCount();
		std::vector<float> boneIds((size_t)vcount * 4);
		std::vector<float> boneWeights((size_t)vcount * 4);
		for (int i = 0; i < vcount; ++i) {
			const VertexSkin& s = cpu.skin[(size_t)i];
			for (int k = 0; k < 4; ++k) {
				boneIds[(size_t)i * 4 + k] = (float)s.joints[k];
				boneWeights[(size_t)i * 4 + k] = s.weights[k];
			}
		}
		rlEnableVertexArray(vaoId);
		unsigned int idVbo = rlLoadVertexBuffer(boneIds.data(),
			(int)(boneIds.size() * sizeof(float)), false);
		rlSetVertexAttribute(kBoneIdsLoc, 4, RL_FLOAT, false, 0, 0);
		rlEnableVertexAttribute(kBoneIdsLoc);
		unsigned int wVbo = rlLoadVertexBuffer(boneWeights.data(),
			(int)(boneWeights.size() * sizeof(float)), false);
		rlSetVertexAttribute(kBoneWeightsLoc, 4, RL_FLOAT, false, 0, 0);
		rlEnableVertexAttribute(kBoneWeightsLoc);
		rlDisableVertexArray();
		return GLSkinMesh{
			.joints_vbo = idVbo,
			.weights_vbo = wVbo
		};
	}

	void GLRenderer::ApplyMaterial(const BaseMaterial& material, const ::Shader& shader,
		AssetManager& assets)
	{
		auto& locs = m_materialLocs[shader.id];
		auto findLoc = [&](const std::string& name) {
			auto it = locs.find(name);
			if (it != locs.end()) {
				return it->second;
			}
			int loc = rlGetLocationUniform(shader.id, name.c_str());
			locs.emplace(name, loc);
			return loc;
		};
		for (const auto& [name, value] : material.Uniforms()) {
			int loc = findLoc(name);
			if (loc < 0) {
				continue; // shader doesn't declare this parameter
			}
			std::visit([&](auto&& v) {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, float>)
					rlSetUniform(loc, &v, SHADER_UNIFORM_FLOAT, 1);
				else if constexpr (std::is_same_v<T, int>)
					rlSetUniform(loc, &v, SHADER_UNIFORM_INT, 1);
				else if constexpr (std::is_same_v<T, raylib::Vector2>)
					rlSetUniform(loc, &v, SHADER_UNIFORM_VEC2, 1);
				else if constexpr (std::is_same_v<T, raylib::Vector3>)
					rlSetUniform(loc, &v, SHADER_UNIFORM_VEC3, 1);
				else if constexpr (std::is_same_v<T, raylib::Vector4>)
					rlSetUniform(loc, &v, SHADER_UNIFORM_VEC4, 1);
				}, value);
		}

		// ---- Texture maps: fixed units 0..3 (shadow maps live at 10+, see
		// kShadowTexSlot0). Every sampler the shader declares gets a binding:
		// the material's texture when set, raylib's 1x1 white otherwise -- an
		// unbound sampler would read whatever happens to sit on unit 0.
		static constexpr struct { const char* name; int slot; } kTextureSlots[] = {
			{ "texture0",        0 }, // albedo (raylib's diffuse map name)
			{ "u_texMetalRough", 1 },
			{ "u_texEmissive",   2 },
			{ "u_texOcclusion",  3 },
		};
		const auto& textures = material.Textures();
		for (const auto& entry : kTextureSlots) {
			int loc = findLoc(entry.name);
			if (loc < 0) {
				continue;
			}
			unsigned int glId = rlGetTextureIdDefault();
			auto t = textures.find(entry.name);
			if (t != textures.end() && assets.IsValidTexture(t->second)) {
				glId = assets.GetTexture(t->second).id;
			}
			int slot = entry.slot;
			rlActiveTextureSlot(slot);
			rlEnableTexture(glId);
			rlSetUniform(loc, &slot, SHADER_UNIFORM_INT, 1);
		}
		rlActiveTextureSlot(0);
	}

	void GLRenderer::DrawMeshImmediate(AssetManager& assets, uint32_t meshId,
		const BaseMaterial& material, const raylib::Matrix& transform,
		const Skeleton* skeleton)
	{
		GLGpuMesh& gpu = UploadGpuMesh(assets, meshId);
		if (gpu.mesh.vaoId == 0) {
			return;
		}
		uint32_t shaderId = material.GetShaderId();
		if (skeleton != nullptr) {
			uint32_t skinnedId = assets.GetSkinnedShaderId(shaderId);
			if (skinnedId != AssetManager::Invalid) {
				shaderId = skinnedId;
			}
			else {
				skeleton = nullptr; // no SKINNED variant -> draw unskinned
			}
		}
		if (!assets.IsValidShader(shaderId)) {
			return;
		}
		const ::Shader sh = assets.GetShader(shaderId);
		if (!m_immediateMaterialReady) {
			m_immediateMaterial = ::LoadMaterialDefault();
			m_immediateMaterialReady = true;
		}
		m_immediateMaterial.shader = sh;
		// ::DrawMesh rebinds map 0 (diffuse) itself, so the albedo texture must
		// go through the raylib material; the extra u_tex* samplers keep the
		// unit bindings ApplyMaterial made.
		::Texture2D albedo{ rlGetTextureIdDefault(), 1, 1, 1,
			PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
		auto t = material.Textures().find("texture0");
		if (t != material.Textures().end() && assets.IsValidTexture(t->second)) {
			albedo = assets.GetTexture(t->second);
		}
		m_immediateMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = albedo;
		rlEnableShader(sh.id);
		ApplyMaterial(material, sh, assets);
		if (skeleton != nullptr) {
			// Uniforms stick to the program, so binding before ::DrawMesh (which
			// re-enables the same shader) is safe.
			BindJointMatrices(*skeleton);
		}
		::DrawMesh(gpu.mesh, m_immediateMaterial, transform);
	}

	void GLRenderer::DrawSkybox(AssetManager& assets, uint32_t meshId,
		const BaseMaterial& material, const raylib::Vector3& cameraPos)
	{
		// Inside faces face the camera (no cull) and the sky must never occlude
		// scene geometry (no depth writes). RAII restores both.
		ScopedBackfaceCull cull(false);
		ScopedDepthMask mask(false);
		DrawMeshImmediate(assets, meshId, material,
			raylib::Matrix(MatrixTranslate(cameraPos.x, cameraPos.y, cameraPos.z)));
	}

	void GLRenderer::DrawDebugLines(AssetManager& assets,
		const std::vector<DebugLineVertex>& lines)
	{
		if (lines.empty()) {
			return;
		}
		uint32_t shaderId = assets.GetShaderId("debug_line");
		if (!assets.IsValidShader(shaderId)) {
			return;
		}
		const ::Shader sh = assets.GetShader(shaderId);

		const int bytes = (int)(lines.size() * sizeof(DebugLineVertex));
		if (m_lineVao == 0) {
			m_lineVao = rlLoadVertexArray();
		}
		rlEnableVertexArray(m_lineVao);
		if (m_lineVbo == 0 || lines.size() > m_lineCapacity) {
			if (m_lineVbo != 0) {
				rlUnloadVertexBuffer(m_lineVbo);
			}
			m_lineVbo = rlLoadVertexBuffer(lines.data(), bytes, true); // dynamic
			m_lineCapacity = lines.size();
			rlSetVertexAttribute(0, 3, RL_FLOAT, 0, (int)sizeof(DebugLineVertex), 0);
			rlEnableVertexAttribute(0);
			rlSetVertexAttribute(3, 4, RL_UNSIGNED_BYTE, 1, (int)sizeof(DebugLineVertex), 12);
			rlEnableVertexAttribute(3);
		}
		else {
			rlUpdateVertexBuffer(m_lineVbo, lines.data(), bytes, 0);
		}

		rlEnableShader(sh.id);
		// Matrices from the current BeginMode3D (call this inside camera mode).
		raylib::Matrix mvp = raylib::Matrix(rlGetMatrixTransform())
			.Multiply(rlGetMatrixModelview())
			.Multiply(rlGetMatrixProjection());
		rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP], mvp);
		// rlgl's vertex-array draws are triangles-only -> raw GL for lines.
		glDrawArrays(GL_LINES, 0, (GLsizei)lines.size());
		rlDisableVertexArray();
		rlDisableShader();
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

	void GLRenderer::DrawBatches(const std::vector<Batch>& batches,
		size_t batchCount,
		AssetManager& assets,
		RenderStats& stats,
		const SceneLights* lights)
	{
		stats.materialCount = assets.materialCount();
		constexpr size_t kInstanceThreshold = 4;

		const raylib::Matrix matView(rlGetMatrixModelview());
		const raylib::Matrix matProjection(rlGetMatrixProjection());
		const raylib::Matrix matStack(rlGetMatrixTransform()); // rlPushMatrix stack, normally identity
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
			const bool skinned = (batch.skeleton != nullptr);
			uint32_t skinShaderId = skinned ? assets.GetSkinnedShaderId(baseShaderId)
				: AssetManager::Invalid;
			uint32_t instShaderId = skinned ? AssetManager::Invalid
				: assets.GetInstancedShaderId(baseShaderId);
			const bool instanced =
				(count >= kInstanceThreshold && instShaderId != AssetManager::Invalid);

			uint32_t useShaderId = baseShaderId;
			if (skinned && skinShaderId != AssetManager::Invalid) useShaderId = skinShaderId;
			else if (instanced) useShaderId = instShaderId;
			raylib::Shader& shader = assets.GetShader(useShaderId);
			const ::Shader sh = shader;
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
				int viewPosLoc = rlGetLocationUniform(sh.id, "u_viewPos");
				if (viewPosLoc != -1) {
					rlSetUniform(viewPosLoc, camPos, SHADER_UNIFORM_VEC3, 1);
				}
				if (sh.locs[SHADER_LOC_MAP_DIFFUSE] != -1) {
					rlActiveTextureSlot(0);
					rlEnableTexture(rlGetTextureIdDefault());
					int slot0 = 0;
					rlSetUniform(sh.locs[SHADER_LOC_MAP_DIFFUSE], &slot0, SHADER_UNIFORM_INT, 1);
				}
				if (lights != nullptr) {
					BindLights(shader, *lights);
					BindShadow(shader, *lights);
				}
			}

			// Per-material face culling. rlgl only toggles culling on/off (not
			// the face), so drive GL directly here. glCullFace/GL_CULL_FACE
			// persist on the context; the restore after the loop puts them back
			// to raylib's default (cull GL_BACK, culling enabled).
			switch (batch.material->GetCullMode()) {
			case CullMode::None:
				rlDisableBackfaceCulling();
				break;
			case CullMode::Front:
				rlEnableBackfaceCulling();
				glCullFace(GL_FRONT);
				break;
			case CullMode::Back:
			default:
				rlEnableBackfaceCulling();
				glCullFace(GL_BACK);
				break;
			}

			ApplyMaterial(*batch.material, sh, assets);
			GLGpuMesh& mesh = UploadGpuMesh(assets, batch.meshId);
			if (!rlEnableVertexArray(mesh.mesh.vaoId)) {
				continue; // invalid mesh id / failed upload -> nothing to draw
			}
			if (instanced && !skinned) {
				UploadInstanceTransforms(batch.transforms);
				SetupInstanceAttributes(sh, m_instanceVbo);
				if (sh.locs[SHADER_LOC_MATRIX_NORMAL] != -1) {
					rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_NORMAL], MatrixIdentity());
				}
				rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP], viewProjStack);
				if (mesh.mesh.indices != NULL) {
					rlDrawVertexArrayElementsInstanced(0, mesh.mesh.triangleCount * 3, 0, (int)count);
				}
				else {
					rlDrawVertexArrayInstanced(0, mesh.mesh.vertexCount, (int)count);
				}
				stats.drawCalls++;
				stats.triangles += (uint32_t)mesh.mesh.triangleCount * (uint32_t)count;
				stats.vertices += (uint32_t)mesh.mesh.vertexCount * (uint32_t)count;
			}
			else {
				if (skinned && batch.skeleton) {
					BindJointMatrices(*batch.skeleton);
				}
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
					if (mesh.mesh.indices != NULL) {
						rlDrawVertexArrayElements(0, mesh.mesh.triangleCount * 3, 0);
					}
					else {
						rlDrawVertexArray(0, mesh.mesh.vertexCount);
					}
					stats.drawCalls++;
					stats.triangles += (uint32_t)mesh.mesh.triangleCount;
					stats.vertices += (uint32_t)mesh.mesh.vertexCount;
				}
			}

			rlDisableVertexArray();
		}
		// Restore the pipeline default so later passes (skybox, gizmo, shadow)
		// don't inherit a stray cull face/state from the last batch drawn.
		rlEnableBackfaceCulling();
		glCullFace(GL_BACK);
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
		const uint32_t skinDepthId = assets.GetSkinnedShaderId(depthShaderId);
		const bool pointMode = linearDistance && lightPos != nullptr;
		uint32_t activeProgram = 0;
		for (size_t b = 0; b < batchCount; ++b) {
			const Batch& batch = batches[b];
			if (batch.meshId == UINT32_MAX || !batch.material || !batch.material->CastsShadow()) {
				continue; // non-casters don't write to the shadow map
			}
			const size_t count = batch.transforms.size();
			const bool skinned = (batch.skeleton != nullptr)
				&& (skinDepthId != AssetManager::Invalid);
			const bool instanced = !skinned
				&& (count >= kInstanceThreshold && instDepthId != AssetManager::Invalid);

			uint32_t useShaderId = depthShaderId;
			if (skinned) useShaderId = skinDepthId;
			else if (instanced) useShaderId = instDepthId;
			const ::Shader sh = assets.GetShader(useShaderId);
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

			GLGpuMesh& mesh = UploadGpuMesh(assets, batch.meshId);
			if (!rlEnableVertexArray(mesh.mesh.vaoId)) {
				continue; // depth pass skips meshes without a VAO
			}
			if (skinned) {
				// Per-batch: each skinned batch carries its own skeleton.
				BindJointMatrices(*batch.skeleton);
			}

			if (instanced) {
				// mvp = lightViewProj only; per-instance model from the attribute.
				UploadInstanceTransforms(batch.transforms);
				SetupInstanceAttributes(sh, m_instanceVbo);
				rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP], lvp);
				if (mesh.mesh.indices != NULL) {
					rlDrawVertexArrayElementsInstanced(0, mesh.mesh.triangleCount * 3, 0, (int)count);
				}
				else {
					rlDrawVertexArrayInstanced(0, mesh.mesh.vertexCount, (int)count);
				}
			}
			else {
				for (const raylib::Matrix& t : batch.transforms) {
					rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MVP],
						MatrixMultiply(t, lvp));
					if (pointMode && sh.locs[SHADER_LOC_MATRIX_MODEL] != -1) {
						rlSetUniformMatrix(sh.locs[SHADER_LOC_MATRIX_MODEL], t);
					}
					if (mesh.mesh.indices != NULL) {
						rlDrawVertexArrayElements(0, mesh.mesh.triangleCount * 3, 0);
					}
					else {
						rlDrawVertexArray(0, mesh.mesh.vertexCount);
					}
				}
			}
			rlDisableVertexArray();
		}
		rlDisableShader();
	}
}