#include "GLTFImporter.hpp"
#include "engine/AssetManager.hpp"
#include "engine/Logger.hpp"
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

	static bool PrimitiveToMesh(const tinygltf::Model& model,
		const tinygltf::Primitive& prim, ::Mesh& out, std::string& message)
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
		// NOTE: `type` (VEC3) va `componentType` (FLOAT) la 2 field khac nhau,
		// va dieu kien fail phai la || (sai MOT trong hai la loai).
		if (pos_accessor.type != TINYGLTF_TYPE_VEC3 ||
			pos_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) {
			message = "Position not float vec3";
			return false;
		}
		// raylib Mesh chi co index 16-bit: primitive to hon phai split, chua lam.
		if (prim.indices >= 0 && pos_accessor.count > 65535) {
			message = "more than 65535 vertices (raylib indices are 16-bit)";
			return false;
		}
		// 1 buffer view co the chua nhieu accessor -> cong ca 2 tang byteOffset.
		const unsigned char* pos_data =
			pos_buffer.data.data() +
			pos_bufferview.byteOffset +
			pos_accessor.byteOffset;
		// MemAlloc (khong phai malloc): UnloadMesh free bang RL_FREE, allocator
		// phai khop mot cap.
		out.vertices = (float*)MemAlloc((unsigned int)(sizeof(float) * pos_accessor.count * 3));
		uint32_t pos_stride = pos_accessor.ByteStride(pos_bufferview);
		for (size_t i = 0; i < pos_accessor.count; ++i)
		{
			memcpy(
				out.vertices + i * 3,
				pos_data + i * pos_stride,
				sizeof(float) * 3);
		}
		// vertexCount la SO VERTEX, khong phai so float (dung *3).
		out.vertexCount = (int)pos_accessor.count;

		// ---- NORMAL (tuy chon: float vec3) ----
		auto it_normal = prim.attributes.find("NORMAL");
		if (it_normal != prim.attributes.end()) {
			const tinygltf::Accessor& normal_accessor = accessors[it_normal->second];
			const tinygltf::BufferView& normal_bufferview = model.bufferViews[normal_accessor.bufferView];
			const tinygltf::Buffer& normal_buffer = model.buffers[normal_bufferview.buffer];
			// check accessor cua CHINH normal, va chi bo qua attribute (khong fail ca mesh)
			if (normal_accessor.type == TINYGLTF_TYPE_VEC3 &&
				normal_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
				normal_accessor.count == pos_accessor.count) {
				const unsigned char* normal_data =
					normal_buffer.data.data() +
					normal_bufferview.byteOffset +
					normal_accessor.byteOffset;
				out.normals = (float*)MemAlloc((unsigned int)(sizeof(float) * normal_accessor.count * 3));
				uint32_t normal_stride = normal_accessor.ByteStride(normal_bufferview);
				for (size_t i = 0; i < normal_accessor.count; ++i)
				{
					memcpy(
						out.normals + i * 3,
						normal_data + i * normal_stride,
						sizeof(float) * 3);
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
				out.texcoords = (float*)MemAlloc((unsigned int)(sizeof(float) * textcoord_0_accessor.count * 2));
				uint32_t textcoord_0_stride = textcoord_0_accessor.ByteStride(textcoord_0_bufferview);
				for (size_t i = 0; i < textcoord_0_accessor.count; ++i)
				{
					memcpy(
						out.texcoords + i * 2,
						textcoord_0_data + i * textcoord_0_stride,
						sizeof(float) * 2);
				}
			}
		}

		// ---- TEXCOORD_1 (tuy chon: float vec2 -> texcoords2) ----
		auto it_textcoord_1 = prim.attributes.find("TEXCOORD_1");
		if (it_textcoord_1 != prim.attributes.end()) {
			const tinygltf::Accessor& textcoord_1_accessor = accessors[it_textcoord_1->second];
			const tinygltf::BufferView& textcoord_1_bufferview = model.bufferViews[textcoord_1_accessor.bufferView];
			const tinygltf::Buffer& textcoord_1_buffer = model.buffers[textcoord_1_bufferview.buffer];
			if (textcoord_1_accessor.type == TINYGLTF_TYPE_VEC2 &&
				textcoord_1_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
				textcoord_1_accessor.count == pos_accessor.count) {
				const unsigned char* textcoord_1_data =
					textcoord_1_buffer.data.data() +
					textcoord_1_bufferview.byteOffset +
					textcoord_1_accessor.byteOffset;
				// texcoords2, khong phai texcoords (bug cu: alloc mot noi, memcpy noi khac)
				out.texcoords2 = (float*)MemAlloc((unsigned int)(sizeof(float) * textcoord_1_accessor.count * 2));
				uint32_t textcoord_1_stride = textcoord_1_accessor.ByteStride(textcoord_1_bufferview);
				for (size_t i = 0; i < textcoord_1_accessor.count; ++i)
				{
					memcpy(
						out.texcoords2 + i * 2,
						textcoord_1_data + i * textcoord_1_stride,
						sizeof(float) * 2);
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

			out.indices = (unsigned short*)MemAlloc(
				(unsigned int)(sizeof(unsigned short) * index_accessor.count));
			for (size_t i = 0; i < index_accessor.count; ++i)
			{
				const unsigned char* p = index_data + i * index_stride;
				switch (index_accessor.componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					out.indices[i] = (unsigned short)(*p);
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					out.indices[i] = *(const unsigned short*)p;
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					out.indices[i] = (unsigned short)(*(const unsigned int*)p);
					break;
				default:
					// don dep het nhung gi da cap phat roi moi fail
					MemFree(out.indices);
					MemFree(out.vertices);
					if (out.normals) MemFree(out.normals);
					if (out.texcoords) MemFree(out.texcoords);
					if (out.texcoords2) MemFree(out.texcoords2);
					out = ::Mesh{};
					message = "unsupported index component type";
					return false;
				}
			}
			out.triangleCount = (int)(index_accessor.count / 3);
		}
		else {
			out.triangleCount = out.vertexCount / 3; // non-indexed triangle soup
		}
		return true;
	}

	ModelAsset ImportGLTF(const std::filesystem::path& modelPath, AssetManager& assets)
	{
		ModelAsset out;

		tinygltf::TinyGLTF loader;
		// Decode embedded images via raylib (LoadImageWithRaylib). NOTE: decoded
		// pixels live inside tinygltf::Model until it is destroyed at the end of
		// this function -- upload them to GPU textures HERE (material phase) and
		// never keep the Model alive longer than the import.
		//loader.SetImageLoader(&LoadImageWithRaylib, nullptr);
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
		auto t = model.defaultScene;

		auto printNode = [&](const tinygltf::Scene& scene) -> void {
			for (size_t i = 0; i < scene.nodes.size(); i++) {
				//std::cout << "node.name : " << scene.nodes[i] << std::endl;
				Logger::TraceLog(LOG_TRACE, std::format("node.name : {}", scene.nodes[i])); 
			}
		};
		const auto& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0]; 
		printNode(scene);
		
		std::function<void(const tinygltf::Model&, int)> traverseNode;

		traverseNode = [&](const tinygltf::Model& model, int nodeIndex)
		{
			const auto& node = model.nodes[nodeIndex];
			Logger::TraceLog(LOG_TRACE,std::format("node name : {}", node.name));
			for (int child : node.children)
			{
				traverseNode(model, child);
			}
		};

		for (int nodeIndex : scene.nodes)
		{
			traverseNode(model,nodeIndex);
		}
		int skipped = 0;
		for (int mi = 0; mi < (int)model.meshes.size(); ++mi) {
			const tinygltf::Mesh& gm = model.meshes[mi];
			for (const tinygltf::Primitive& prim : gm.primitives) {
				::Mesh m{};
				std::string why;
				if (!PrimitiveToMesh(model, prim, m, why)) {
					Logger::TraceLog(LOG_WARNING, std::format(
						"[GLTF] mesh '{}' primitive skipped: {}", gm.name, why));
					skipped++;
					continue;
				}
				::UploadMesh(&m, false); 
				uint32_t meshIndex = assets.AddMesh(m); 
				out.meshIds.push_back(meshIndex);
				out.gltfMeshIndex.push_back(mi);
				out.meshMaterial.push_back(prim.material);
				out.meshName.insert({ meshIndex,gm.name }); 
			}
		}
		Logger::TraceLog(LOG_INFO, std::format(
			"[GLTF] {}: {} primitives converted, {} skipped",
			modelPath.filename().string(), out.meshIds.size(), skipped));
		return out;
	}
}