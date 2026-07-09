#pragma once
#ifndef _MESH_CPU_HPP_
#define _MESH_CPU_HPP_
#include <vector>
#include <cstdint>
#include <raylib-cpp.hpp>

namespace Long {

	struct VertexPNT {
		float px{ 0 }, py{ 0 }, pz{ 0 }; // position
		float nx{ 0 }, ny{ 0 }, nz{ 0 }; // normal
		float u{ 0 }, v{ 0 };            // texcoord
	};

	//JOINT_0, WEIGHT_0
	struct VertexSkin {
		uint32_t joints[4]{ 0, 0, 0, 0 };
		float    weights[4]{ 0, 0, 0, 0 };
	};
	struct MeshCPU {
		std::vector<VertexPNT> vertices;
		std::vector<uint32_t> indices;
		std::vector<VertexSkin> skin;
		bool IsSkinned() const { return skin.size() == vertices.size() && !skin.empty(); }
		int VertexCount() const { return (int)vertices.size(); }
		int TriangleCount() const {
			return indices.empty() ? (int)vertices.size() / 3
								   : (int)indices.size() / 3;
		}
		bool IsValid() const { return !vertices.empty(); }

		// Local-space AABB over all vertices (colliders / WorldBoundsSystem).
		raylib::BoundingBox Bounds() const {
			if (vertices.empty()) {
				return raylib::BoundingBox({ 0, 0, 0 }, { 0, 0, 0 });
			}
			raylib::Vector3 mn{ vertices[0].px, vertices[0].py, vertices[0].pz };
			raylib::Vector3 mx = mn;
			for (const VertexPNT& vert : vertices) {
				if (vert.px < mn.x) mn.x = vert.px; if (vert.px > mx.x) mx.x = vert.px;
				if (vert.py < mn.y) mn.y = vert.py; if (vert.py > mx.y) mx.y = vert.py;
				if (vert.pz < mn.z) mn.z = vert.pz; if (vert.pz > mx.z) mx.z = vert.pz;
			}
			return raylib::BoundingBox(mn, mx);
		}

		// Copy the CPU arrays out of a raylib mesh (GenMeshCube/Sphere/...). Only
		// reads -- the caller still owns/unloads the raylib mesh afterwards.
		static MeshCPU FromRaylib(const ::Mesh& m) {
			MeshCPU out;
			if (m.vertexCount <= 0 || m.vertices == nullptr) {
				return out;
			}
			out.vertices.resize((size_t)m.vertexCount);
			for (int i = 0; i < m.vertexCount; ++i) {
				VertexPNT& dst = out.vertices[(size_t)i];
				dst.px = m.vertices[i * 3 + 0];
				dst.py = m.vertices[i * 3 + 1];
				dst.pz = m.vertices[i * 3 + 2];
				if (m.normals != nullptr) {
					dst.nx = m.normals[i * 3 + 0];
					dst.ny = m.normals[i * 3 + 1];
					dst.nz = m.normals[i * 3 + 2];
				}
				if (m.texcoords != nullptr) {
					dst.u = m.texcoords[i * 2 + 0];
					dst.v = m.texcoords[i * 2 + 1];
				}
			}
			if (m.indices != nullptr && m.triangleCount > 0) {
				out.indices.resize((size_t)m.triangleCount * 3);
				for (size_t i = 0; i < out.indices.size(); ++i) {
					out.indices[i] = (uint32_t)m.indices[i];
				}
			}
			return out;
		}
	};
}
#endif // !_MESH_CPU_HPP_
