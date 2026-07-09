#include "GLTFImporter.hpp"
#include "engine/AssetManager.hpp"
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

	ModelAsset ImportGLTF(const std::filesystem::path& modelPath, AssetManager& assets)
	{
		ModelAsset out;

		tinygltf::TinyGLTF loader;
		loader.SetImageLoader(
			[](tinygltf::Image*, int, std::string*, std::string*,
				int, int, const unsigned char*, int, void*) { return true; },
			nullptr);

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
						uint32_t meshIndex = assets.AddMesh(std::move(m));
						new_node.meshId = (int)meshIndex; 
					}
				}
				raylib::Vector3 translation{ 0.0f, 0.0f, 0.0f };
				raylib::Vector3 scale{ 1.0f, 1.0f, 1.0f };
				raylib::Quaternion quat{ 0.0f, 0.0f, 0.0f, 1.0f };
				if (node.skin >= 0) {
				}
				else if (!node.matrix.empty()) {
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
		return out;
	}
}