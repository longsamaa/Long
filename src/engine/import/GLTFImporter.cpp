#include "GLTFImporter.hpp"
#include "engine/Logger.hpp"
#include "core/math/transform.hpp"
#include <format>
#include <iostream>
#include <cstring>   // memcpy
#include <functional>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX
#endif
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "tiny_gltf.h"

namespace Long {
	//CALLBACK LOAD TEXTURE
	static bool LoadImageWithRaylib(tinygltf::Image* image, const int imageIdx,
		std::string* err, std::string* /*warn*/, int /*reqWidth*/, int /*reqHeight*/,
		const unsigned char* bytes, int size, void* /*user*/)
	{
		// Core glTF only allows PNG or JPEG; sniff the real magic bytes so an
		// unknown format (webp/ktx2 via extensions) gives a clear error instead
		// of a silent wrong-decoder failure.
		const char* ext = nullptr;
		if (size >= 4 && bytes[0] == 0x89 && bytes[1] == 'P' && bytes[2] == 'N') {
			ext = ".png";
		}
		else if (size >= 3 && bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) {
			ext = ".jpg"; // needs SUPPORT_FILEFORMAT_JPG=ON in raylib (see CMakeLists)
		}
		if (ext == nullptr) {
			if (err) {
				*err += std::format(
					"image {}: unsupported format (magic {:02X} {:02X} {:02X}), "
					"only PNG/JPEG are handled\n",
					imageIdx, bytes[0], bytes[1], size >= 3 ? bytes[2] : 0);
			}
			return false;
		}
		::Image img = ::LoadImageFromMemory(ext, bytes, size);
		if (img.data == nullptr) {
			if (err) {
				*err += std::format(
					"image {} ({}) failed to decode -- if it's a .jpg, check that "
					"raylib was built with SUPPORT_FILEFORMAT_JPG\n", imageIdx, ext);
			}
			return false;
		}
		// Normalize to RGBA8 so the copy below is one flat block.
		::ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
		image->width = img.width;
		image->height = img.height;
		image->component = 4;
		image->bits = 8;
		image->pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		const unsigned char* px = (const unsigned char*)img.data;
		image->image.assign(px, px + (size_t)img.width * img.height * 4);
		::UnloadImage(img);
		return true;
	}

	// One glTF primitive -> one MeshCPU (pure CPU data, NO GL here!). Vertices
	// stay in NODE-LOCAL space -- world placement is the entity Transform's job.
	// The GL backend uploads/caches the GPU side later, so the importer no longer
	// touches MemAlloc/UploadMesh at all.
	static bool PrimitiveToMesh(const tinygltf::Model& model,
		const tinygltf::Primitive& prim, MeshCPU& out, std::string& message)
	{
		if (prim.mode != TINYGLTF_MODE_TRIANGLES) {
			message = "Not triangle mode!";
			return false;
		}
		const auto& accessors = model.accessors;

		// ---- POSITION (bat buoc: float vec3) ----
		auto it_pos = prim.attributes.find("POSITION");
		if (it_pos == prim.attributes.end()) {
			message = "Not have position";
			return false;
		}
		const tinygltf::Accessor& pos_accessor = accessors[it_pos->second];
		const tinygltf::BufferView& pos_bufferview = model.bufferViews[pos_accessor.bufferView];
		const tinygltf::Buffer& pos_buffer = model.buffers[pos_bufferview.buffer];
		if (pos_accessor.type != TINYGLTF_TYPE_VEC3 ||
			pos_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
			message = "Position not float vec3";
			return false;
		}
		// raylib GPU mesh chi co index 16-bit: primitive to hon phai split, chua lam.
		if (prim.indices >= 0 && pos_accessor.count > 65535) {
			message = "more than 65535 vertices (raylib indices are 16-bit)";
			return false;
		}
		const unsigned char* pos_data =
			pos_buffer.data.data() +
			pos_bufferview.byteOffset +
			pos_accessor.byteOffset;
		uint32_t pos_stride = pos_accessor.ByteStride(pos_bufferview);

		// Interleave thang vao VertexPNT (position truoc, normal/uv dien sau).
		out.vertices.assign(pos_accessor.count, VertexPNT{});
		for (size_t i = 0; i < pos_accessor.count; ++i)
		{
			const float* p = (const float*)(pos_data + i * pos_stride);
			out.vertices[i].px = p[0];
			out.vertices[i].py = p[1];
			out.vertices[i].pz = p[2];
		}

		// ---- NORMAL (tuy chon: float vec3) ----
		auto it_normal = prim.attributes.find("NORMAL");
		if (it_normal != prim.attributes.end()) {
			const tinygltf::Accessor& normal_accessor = accessors[it_normal->second];
			const tinygltf::BufferView& normal_bufferview = model.bufferViews[normal_accessor.bufferView];
			const tinygltf::Buffer& normal_buffer = model.buffers[normal_bufferview.buffer];
			if (normal_accessor.type == TINYGLTF_TYPE_VEC3 &&
				normal_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
				normal_accessor.count == pos_accessor.count) {
				const unsigned char* normal_data =
					normal_buffer.data.data() +
					normal_bufferview.byteOffset +
					normal_accessor.byteOffset;
				uint32_t normal_stride = normal_accessor.ByteStride(normal_bufferview);
				for (size_t i = 0; i < normal_accessor.count; ++i)
				{
					const float* n = (const float*)(normal_data + i * normal_stride);
					out.vertices[i].nx = n[0];
					out.vertices[i].ny = n[1];
					out.vertices[i].nz = n[2];
				}
			}
		}

		// ---- TEXCOORD_0 (tuy chon: float vec2) ----
		auto it_textcoord_0 = prim.attributes.find("TEXCOORD_0");
		if (it_textcoord_0 != prim.attributes.end()) {
			const tinygltf::Accessor& textcoord_0_accessor = accessors[it_textcoord_0->second];
			const tinygltf::BufferView& textcoord_0_bufferview = model.bufferViews[textcoord_0_accessor.bufferView];
			const tinygltf::Buffer& textcoord_0_buffer = model.buffers[textcoord_0_bufferview.buffer];
			if (textcoord_0_accessor.type == TINYGLTF_TYPE_VEC2 &&
				textcoord_0_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
				textcoord_0_accessor.count == pos_accessor.count) {
				const unsigned char* textcoord_0_data =
					textcoord_0_buffer.data.data() +
					textcoord_0_bufferview.byteOffset +
					textcoord_0_accessor.byteOffset;
				uint32_t textcoord_0_stride = textcoord_0_accessor.ByteStride(textcoord_0_bufferview);
				for (size_t i = 0; i < textcoord_0_accessor.count; ++i)
				{
					const float* t = (const float*)(textcoord_0_data + i * textcoord_0_stride);
					out.vertices[i].u = t[0];
					out.vertices[i].v = t[1];
				}
			}
		}

		auto it_joints = prim.attributes.find("JOINTS_0");
		auto it_weights = prim.attributes.find("WEIGHTS_0");
		if (it_joints != prim.attributes.end() && it_weights != prim.attributes.end()) {
			const tinygltf::Accessor& ja = accessors[it_joints->second];
			const tinygltf::Accessor& wa = accessors[it_weights->second];
			const tinygltf::BufferView& jbv = model.bufferViews[ja.bufferView];
			const tinygltf::BufferView& wbv = model.bufferViews[wa.bufferView];
			const tinygltf::Buffer& jbuf = model.buffers[jbv.buffer];
			const tinygltf::Buffer& wbuf = model.buffers[wbv.buffer];
			const bool okTypes =
				ja.type == TINYGLTF_TYPE_VEC4 && wa.type == TINYGLTF_TYPE_VEC4 &&
				ja.count == pos_accessor.count && wa.count == pos_accessor.count;
			if (okTypes) {
				const unsigned char* jdata = jbuf.data.data() + jbv.byteOffset + ja.byteOffset;
				const unsigned char* wdata = wbuf.data.data() + wbv.byteOffset + wa.byteOffset;
				uint32_t jstride = ja.ByteStride(jbv);
				uint32_t wstride = wa.ByteStride(wbv);
				out.skin.assign(pos_accessor.count, VertexSkin{});
				for (size_t i = 0; i < pos_accessor.count; ++i) {
					const unsigned char* jp = jdata + i * jstride;
					// Joint indices: u8 or u16 per component.
					for (int k = 0; k < 4; ++k) {
						uint32_t idx = 0;
						if (ja.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
							idx = ((const unsigned char*)jp)[k];
						}
						else { // UNSIGNED_SHORT
							idx = ((const unsigned short*)jp)[k];
						}
						out.skin[i].joints[k] = idx;
					}
					// Weights: float, or normalized u8/u16.
					const unsigned char* wp = wdata + i * wstride;
					for (int k = 0; k < 4; ++k) {
						float w = 0.0f;
						if (wa.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
							w = ((const float*)wp)[k];
						}
						else if (wa.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
							w = ((const unsigned char*)wp)[k] / 255.0f;
						}
						else { // UNSIGNED_SHORT
							w = ((const unsigned short*)wp)[k] / 65535.0f;
						}
						out.skin[i].weights[k] = w;
					}
				}
			}
			else {
				Logger::TraceLog(LOG_WARNING,
					"[GLTF] JOINTS_0/WEIGHTS_0 present but unsupported layout, skipping skin");
			}
		}

		// ---- INDICES (tuy chon; khong co thi la triangle soup) ----
		if (prim.indices >= 0) {
			const tinygltf::Accessor& index_accessor = accessors[prim.indices];
			const tinygltf::BufferView& index_bufferview = model.bufferViews[index_accessor.bufferView];
			const tinygltf::Buffer& index_buffer = model.buffers[index_bufferview.buffer];
			const unsigned char* index_data =
				index_buffer.data.data() +
				index_bufferview.byteOffset +
				index_accessor.byteOffset;
			uint32_t index_stride = index_accessor.ByteStride(index_bufferview);

			out.indices.resize(index_accessor.count);
			for (size_t i = 0; i < index_accessor.count; ++i)
			{
				const unsigned char* p = index_data + i * index_stride;
				switch (index_accessor.componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					out.indices[i] = (uint32_t)(*p);
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					out.indices[i] = (uint32_t)(*(const unsigned short*)p);
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					out.indices[i] = *(const unsigned int*)p;
					break;
				default:
					out = MeshCPU{};
					message = "unsupported index component type";
					return false;
				}
			}
		}
		return true;
	}

	ModelAsset ImportGLTF(const std::filesystem::path& modelPath)
	{
		ModelAsset out;

		tinygltf::TinyGLTF loader;
		// Decode embedded PNG/JPEG through raylib (stb is compiled out, see the
		// TINYGLTF_NO_STB_* defines above). Images land in model.images as RGBA8.
		loader.SetImageLoader(LoadImageWithRaylib, nullptr);

		tinygltf::Model model;
		std::string err, warn;
		const bool isBinary = modelPath.extension() == ".glb";
		const bool ok = isBinary
			? loader.LoadBinaryFromFile(&model, &err, &warn, modelPath.string())
			: loader.LoadASCIIFromFile(&model, &err, &warn, modelPath.string());

		if (!warn.empty()) {
			Logger::TraceLog(LOG_WARNING,
				std::format("[GLTF] {}: {}", modelPath.filename().string(), warn));
		}
		if (!ok) {
			Logger::TraceLog(LOG_ERROR,
				std::format("[GLTF] failed to load {}: {}",
					modelPath.string(), err.empty() ? "unknown error" : err));
			return out;
		}
		Logger::TraceLog(LOG_INFO,
			std::format("[GLTF] loaded {}: {} meshes, {} materials, {} nodes, {} images",
				modelPath.filename().string(), model.meshes.size(),
				model.materials.size(), model.nodes.size(), model.images.size()));
		const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
		int skipped = 0;
		// Map a glTF node index -> our node index (our traversal order differs from
		// glTF's flat array). Needed to remap skin joints, which reference glTF
		// node indices, into our node list.
		std::vector<int> gltfToOurs(model.nodes.size(), -1);

		std::function<void(const tinygltf::Model&,
			int,
			std::vector<GLTFNode>&,
			int)> traverseNode;
		traverseNode = [&](const tinygltf::Model& model,
			int nodeIndexGLTF,
			std::vector<GLTFNode>& nodes,
			int parent_node)
			{
				const auto& node = model.nodes[nodeIndexGLTF];
				//create 1 node
				GLTFNode new_node;
				new_node.name = node.name;
				new_node.id_parent = parent_node;
				if (node.mesh >= 0) {
					const tinygltf::Mesh& gm = model.meshes[node.mesh];
					for (const tinygltf::Primitive& prim : gm.primitives) {
						MeshCPU m;
						std::string why;
						if (!PrimitiveToMesh(model, prim, m, why)) {
							Logger::TraceLog(LOG_WARNING, std::format(
								"[GLTF] mesh '{}' primitive skipped: {}", gm.name, why));
							skipped++;
							continue;
						}
						// Meshes stay inside the ModelAsset (LOCAL index); the
						// AssetManager only sees them at instantiation time.
						new_node.meshIds.push_back((int)out.meshes.size());
						out.meshMaterials.push_back(prim.material); // LOCAL material, -1 = none
						out.meshes.emplace_back(std::move(m)); // keep ALL primitives
					}
				}
				raylib::Vector3 translation{ 0.0f, 0.0f, 0.0f };
				raylib::Vector3 scale{ 1.0f, 1.0f, 1.0f };
				raylib::Quaternion quat{ 0.0f, 0.0f, 0.0f, 1.0f };
				new_node.skinId = node.skin;
				if (!node.matrix.empty()) {
					raylib::Matrix matrix = VectorMatrixToRaylibMatrix(node.matrix);
					auto transform = DecomposeToTransform(matrix);
					translation = transform.position;
					scale = transform.scale;
					quat = transform.quaternion;
				}
				else {
					if (node.translation.size() == 3) translation = VectorToRaylibVector3(node.translation);
					if (node.scale.size() == 3)       scale = VectorToRaylibVector3(node.scale);
					if (node.rotation.size() == 4)    quat = VectorToRaylibQuaternion(node.rotation);
				}
				new_node.transform = {
					.pos = translation,
					.scale = scale,
					.quaternion = quat
				};
				new_node.index = (int)nodes.size();
				gltfToOurs[nodeIndexGLTF] = new_node.index;
				nodes.emplace_back(new_node);

				if (parent_node >= 0 && parent_node < (int)nodes.size()) {
					auto& parent = nodes[parent_node];
					auto it_child = std::find(parent.children.begin(), parent.children.end(), new_node.index);
					if (it_child == parent.children.end()) {
						parent.children.emplace_back(new_node.index);
					}
				}

				Logger::TraceLog(LOG_TRACE, std::format("node name : {}", node.name));
				for (int childIndex : node.children)
				{
					traverseNode(model,
						childIndex,
						nodes,
						new_node.index);
				}
			};

		raylib::Matrix parent_matrix = raylib::Matrix::Identity();
		std::vector<GLTFNode> nodes;
		for (int nodeIndex : scene.nodes)
		{
			traverseNode(model,
				nodeIndex,
				nodes,
				-1);
		}
		out.nodes = nodes;
		out.skins.reserve(model.skins.size());
		for (const tinygltf::Skin& gskin : model.skins) {
			GLTFSkin skin;
			skin.joints.reserve(gskin.joints.size());
			for (int gjoint : gskin.joints) {
				int ours = (gjoint >= 0 && gjoint < (int)gltfToOurs.size())
					? gltfToOurs[gjoint] : -1;
				if (ours < 0) {
					Logger::TraceLog(LOG_WARNING, std::format(
						"[GLTF] skin joint (gltf node {}) not in scene traversal -> -1", gjoint));
				}
				skin.joints.push_back(ours);
			}
			// Inverse-bind matrices (optional; absent = identity per joint).
			skin.inverseBind.assign(skin.joints.size(), raylib::Matrix::Identity());
			if (gskin.inverseBindMatrices >= 0) {
				const tinygltf::Accessor& iba = model.accessors[gskin.inverseBindMatrices];
				const tinygltf::BufferView& bv = model.bufferViews[iba.bufferView];
				const tinygltf::Buffer& buf = model.buffers[bv.buffer];
				if (iba.type == TINYGLTF_TYPE_MAT4 &&
					iba.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
					iba.count == skin.joints.size()) {
					const unsigned char* data = buf.data.data() + bv.byteOffset + iba.byteOffset;
					uint32_t stride = iba.ByteStride(bv);
					for (size_t j = 0; j < iba.count; ++j) {
						const float* m = (const float*)(data + j * stride);
						// glTF MAT4 is column-major, same as raylib ::Matrix -> copy fields.
						::Matrix mat;
						mat.m0 = m[0];  mat.m1 = m[1];  mat.m2 = m[2];  mat.m3 = m[3];
						mat.m4 = m[4];  mat.m5 = m[5];  mat.m6 = m[6];  mat.m7 = m[7];
						mat.m8 = m[8];  mat.m9 = m[9];  mat.m10 = m[10]; mat.m11 = m[11];
						mat.m12 = m[12]; mat.m13 = m[13]; mat.m14 = m[14]; mat.m15 = m[15];
						skin.inverseBind[j] = raylib::Matrix(mat);
					}
				}
				else {
					Logger::TraceLog(LOG_WARNING,
						"[GLTF] inverseBindMatrices unsupported layout, using identity");
				}
			}
			out.skins.push_back(std::move(skin));
		}
		for (GLTFNode& n : out.nodes) {
			if (n.skinId >= (int)out.skins.size()) {
				n.skinId = -1; // out-of-range guard
			}
		}

		// ---- Images: raylib callback already normalized everything to RGBA8 ----
		out.images.reserve(model.images.size());
		for (const tinygltf::Image& gi : model.images) {
			GLTFImage img;
			if (gi.width > 0 && gi.height > 0 &&
				gi.image.size() == (size_t)gi.width * gi.height * 4) {
				img.width = gi.width;
				img.height = gi.height;
				img.pixels = gi.image;
			} // else: decode failed -> keep empty, instantiation skips it
			out.images.push_back(std::move(img));
		}

		// ---- Materials: PBR metallic-roughness factors + image references ----
		// glTF indirection is texture -> image; we resolve it here so the asset
		// only carries image indices.
		auto texToImage = [&](int texIndex) -> int {
			if (texIndex < 0 || texIndex >= (int)model.textures.size()) return -1;
			const int src = model.textures[texIndex].source;
			return (src >= 0 && src < (int)model.images.size()) ? src : -1;
		};
		out.materials.reserve(model.materials.size());
		for (const tinygltf::Material& gm : model.materials) {
			GLTFMaterial m;
			const auto& pbr = gm.pbrMetallicRoughness;
			if (pbr.baseColorFactor.size() == 4) {
				m.baseColorFactor = {
					(float)pbr.baseColorFactor[0], (float)pbr.baseColorFactor[1],
					(float)pbr.baseColorFactor[2], (float)pbr.baseColorFactor[3] };
			}
			m.metallicFactor = (float)pbr.metallicFactor;
			m.roughnessFactor = (float)pbr.roughnessFactor;
			if (gm.emissiveFactor.size() == 3) {
				m.emissiveFactor = {
					(float)gm.emissiveFactor[0], (float)gm.emissiveFactor[1],
					(float)gm.emissiveFactor[2] };
			}
			m.baseColorImage = texToImage(pbr.baseColorTexture.index);
			m.metallicRoughnessImage = texToImage(pbr.metallicRoughnessTexture.index);
			m.emissiveImage = texToImage(gm.emissiveTexture.index);
			m.occlusionImage = texToImage(gm.occlusionTexture.index);
			m.doubleSided = gm.doubleSided;
			out.materials.push_back(m);
		}

		// ---- Animations: TRS keyframe clips, node targets remapped to OUR indices ----
		out.animations.reserve(model.animations.size());
		for (const tinygltf::Animation& ga : model.animations) {
			GLTFAnimationClip clip;
			clip.name = ga.name.empty()
				? std::format("anim_{}", out.animations.size()) : ga.name;
			for (const tinygltf::AnimationChannel& gc : ga.channels) {
				if (gc.target_path == "weights") {
					Logger::TraceLog(LOG_WARNING, std::format(
						"[GLTF] '{}': morph-target (weights) channel skipped, unsupported", clip.name));
					continue;
				}
				if (gc.sampler < 0 || gc.sampler >= (int)ga.samplers.size()) {
					continue;
				}
				const int ourNode = (gc.target_node >= 0 && gc.target_node < (int)gltfToOurs.size())
					? gltfToOurs[gc.target_node] : -1;
				if (ourNode < 0) {
					Logger::TraceLog(LOG_WARNING, std::format(
						"[GLTF] '{}': channel targets gltf node {} not in scene traversal, skipped",
						clip.name, gc.target_node));
					continue;
				}
				const tinygltf::AnimationSampler& gs = ga.samplers[gc.sampler];
				if (gs.input < 0 || gs.output < 0) {
					continue;
				}
				const tinygltf::Accessor& ia = model.accessors[gs.input];
				const tinygltf::Accessor& oa = model.accessors[gs.output];
				// Only plain float accessors with a bufferView (no sparse storage).
				if (ia.bufferView < 0 || oa.bufferView < 0 ||
					ia.type != TINYGLTF_TYPE_SCALAR ||
					ia.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
					oa.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
					Logger::TraceLog(LOG_WARNING, std::format(
						"[GLTF] '{}': channel with non-float/sparse accessors skipped", clip.name));
					continue;
				}

				GLTFAnimChannel ch;
				ch.node = ourNode;
				const bool rotation = (gc.target_path == "rotation");
				if (gc.target_path == "translation")  ch.path = GLTFAnimChannel::Path::Translation;
				else if (rotation)                    ch.path = GLTFAnimChannel::Path::Rotation;
				else if (gc.target_path == "scale")   ch.path = GLTFAnimChannel::Path::Scale;
				else continue; // unknown target path
				if (oa.type != (rotation ? TINYGLTF_TYPE_VEC4 : TINYGLTF_TYPE_VEC3)) {
					Logger::TraceLog(LOG_WARNING, std::format(
						"[GLTF] '{}': {} channel has wrong output type, skipped",
						clip.name, gc.target_path));
					continue;
				}

				// CUBICSPLINE stores [inTangent, value, outTangent] per key; we keep
				// only the value and fall back to linear interpolation.
				const bool cubic = (gs.interpolation == "CUBICSPLINE");
				ch.interp = (gs.interpolation == "STEP")
					? GLTFAnimChannel::Interp::Step : GLTFAnimChannel::Interp::Linear;
				const size_t keyCount = ia.count;
				const size_t expectedOut = cubic ? keyCount * 3 : keyCount;
				if (keyCount == 0 || oa.count != expectedOut) {
					Logger::TraceLog(LOG_WARNING, std::format(
						"[GLTF] '{}': key/value count mismatch ({} vs {}), channel skipped",
						clip.name, keyCount, oa.count));
					continue;
				}
				if (cubic) {
					Logger::TraceLog(LOG_WARNING, std::format(
						"[GLTF] '{}': CUBICSPLINE downgraded to linear", clip.name));
				}

				const tinygltf::BufferView& ibv = model.bufferViews[ia.bufferView];
				const unsigned char* idata =
					model.buffers[ibv.buffer].data.data() + ibv.byteOffset + ia.byteOffset;
				const uint32_t istride = ia.ByteStride(ibv);
				ch.times.resize(keyCount);
				for (size_t k = 0; k < keyCount; ++k) {
					ch.times[k] = *(const float*)(idata + k * istride);
				}

				const tinygltf::BufferView& obv = model.bufferViews[oa.bufferView];
				const unsigned char* odata =
					model.buffers[obv.buffer].data.data() + obv.byteOffset + oa.byteOffset;
				const uint32_t ostride = oa.ByteStride(obv);
				// Index of the k-th VALUE element (cubicspline keeps the middle of 3).
				auto valueIndex = [cubic](size_t k) { return cubic ? k * 3 + 1 : k; };
				if (rotation) {
					ch.quatKeys.resize(keyCount);
					for (size_t k = 0; k < keyCount; ++k) {
						const float* v = (const float*)(odata + valueIndex(k) * ostride);
						ch.quatKeys[k] = raylib::Quaternion{ v[0], v[1], v[2], v[3] };
					}
				}
				else {
					ch.vec3Keys.resize(keyCount);
					for (size_t k = 0; k < keyCount; ++k) {
						const float* v = (const float*)(odata + valueIndex(k) * ostride);
						ch.vec3Keys[k] = raylib::Vector3{ v[0], v[1], v[2] };
					}
				}
				if (ch.times.back() > clip.duration) {
					clip.duration = ch.times.back();
				}
				clip.channels.push_back(std::move(ch));
			}
			if (!clip.channels.empty()) {
				out.animations.push_back(std::move(clip));
			}
		}
		if (!model.animations.empty()) {
			Logger::TraceLog(LOG_INFO, std::format(
				"[GLTF] imported {}/{} animation clip(s)",
				out.animations.size(), model.animations.size()));
		}
		return out;
	}
}