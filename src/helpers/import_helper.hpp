#pragma once 
#ifndef _IMPORT_MODEL_ASSET_HELPER_HPP_
#define _IMPORT_MODEL_ASSET_HELPER_HPP_
#include <entt/entt.hpp>
#include "core/Components.hpp"
#include "engine/import/GLTFImporter.hpp"
#include "engine/AssetManager.hpp"
#include "engine/Logger.hpp"
namespace Long {
	static void ImportGLTFToScene(entt::registry& registry,
		const std::string& name,
		const ModelAsset& model, 
		AssetManager& asset,
		const std::string& shader) {
		uint32_t shaderId = asset.GetShaderId(shader);
		uint32_t materialId = asset.CreateDefaultMaterial(shaderId); // fallback: primitives without a material
		// Upload images lazily -- only the ones a material actually references.
		std::vector<uint32_t> imageLocalToGlobal(model.images.size(), AssetManager::Invalid);
		auto textureFor = [&](int localImage) -> uint32_t {
			if (localImage < 0 || localImage >= (int)model.images.size()) {
				return AssetManager::Invalid;
			}
			const GLTFImage& img = model.images[localImage];
			if (img.pixels.empty()) {
				return AssetManager::Invalid; // decode failed at import time
			}
			if (imageLocalToGlobal[localImage] == AssetManager::Invalid) {
				// Borrowed view: AddTexture only reads the pixels for the upload.
				::Image view{ (void*)img.pixels.data(), img.width, img.height,
					1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
				imageLocalToGlobal[localImage] = asset.AddTexture(view);
			}
			return imageLocalToGlobal[localImage];
		};
		// One engine material per glTF material: PBR factors + texture maps.
		std::vector<uint32_t> materialLocalToGlobal(model.materials.size(), materialId);
		for (size_t i = 0; i < model.materials.size(); ++i) {
			const GLTFMaterial& gm = model.materials[i];
			uint32_t id = asset.CreateDefaultMaterial(shaderId);
			BaseMaterial& mat = asset.GetMaterial(id);
			mat.SetUniform("u_baseColor", gm.baseColorFactor);
			mat.SetMetallic(gm.metallicFactor);
			mat.SetRoughness(gm.roughnessFactor);
			mat.SetEmissive(gm.emissiveFactor, 1.0f); // glTF: emissive = factor * texture
			// glTF doubleSided -> draw both faces (foliage, cloth, cards).
			if (gm.doubleSided) {
				mat.SetCullMode(CullMode::None);
			}
			uint32_t tex;
			if ((tex = textureFor(gm.baseColorImage)) != AssetManager::Invalid) {
				mat.SetTexture("texture0", tex);
			}
			if ((tex = textureFor(gm.metallicRoughnessImage)) != AssetManager::Invalid) {
				mat.SetTexture("u_texMetalRough", tex);
			}
			if ((tex = textureFor(gm.emissiveImage)) != AssetManager::Invalid) {
				mat.SetTexture("u_texEmissive", tex);
			}
			if ((tex = textureFor(gm.occlusionImage)) != AssetManager::Invalid) {
				mat.SetTexture("u_texOcclusion", tex);
			}
			materialLocalToGlobal[i] = id;
		}
		//Node
		const std::vector<GLTFNode>& nodes = model.nodes;
		//Skeleton
		const std::vector<GLTFSkin>& skins = model.skins;
		// Register the asset's meshes with the AssetManager. node.meshIds are
		// LOCAL indices into model.meshes; this remap is the only place local
		// ids meet runtime ids (the ModelAsset itself stays reusable).
		std::vector<uint32_t> meshLocalToGlobal(model.meshes.size(), AssetManager::Invalid);
		for (size_t i = 0; i < model.meshes.size(); ++i) {
			meshLocalToGlobal[i] = asset.AddMesh(MeshCPU(model.meshes[i])); // copy: keep the asset intact
		}
		//create entt 
		entt::entity root = registry.create(); 
		if (!name.empty()) {
			registry.emplace<Name>(root, name);
		}
		registry.emplace<Transform>(root, Transform{}); 
		Hierarchy& h_root = registry.emplace<Hierarchy>(root, Hierarchy{ entt::null,{} });
		
		
		std::vector<entt::entity> entities; 
		for (int i = 0; i < nodes.size(); ++i) {
			entt::entity e = registry.create(); 
			entities.emplace_back(e); 
			registry.emplace<Hierarchy>(e, Hierarchy{ entt::null,{} }); 
		}

		std::vector<SkinnedMeshRenderer> skinnedMeshRenderComponent{};
		if (!model.animations.empty()) {
			uint32_t skinIndex = 0;
			for (const auto& skin : skins) {
				const auto& joins = skin.joints;
				const auto& inverseBind = skin.inverseBind;
				std::string str_index = std::to_string(skinIndex);
				SkinnedMeshRenderer skinnedComponent{ skinIndex++,entt::null };
				if (joins.size() != inverseBind.size()) {
					skinnedMeshRenderComponent.push_back(skinnedComponent); // keep index aligned with skins[]
					continue;
				}
				entt::entity skeleton_entt = registry.create();
				Hierarchy& h = registry.emplace<Hierarchy>(skeleton_entt, Hierarchy{ root,{} });
				auto it = std::find(h_root.children.begin(), h_root.children.end(), skeleton_entt);
				if (it == h_root.children.end()) {
					h_root.children.emplace_back(skeleton_entt);
				}
				Skeleton skeleton;
				for (int i = 0; i < joins.size(); ++i) {
					const auto& join = joins[i];
					if (join < 0 || join >= nodes.size()) {
						continue;
					}
					entt::entity join_entt = entities[join];
					skeleton.joints.emplace_back(join_entt);
					const auto& inverse = inverseBind[i];
					skeleton.inverseBind.emplace_back(inverse);
				}
				skeleton.jointMatrices.resize(skeleton.joints.size());
				//Skeleton
				registry.emplace<Skeleton>(skeleton_entt, skeleton);
				registry.emplace<Name>(skeleton_entt, std::format("skeleton_{}", str_index));
				skinnedComponent.skeleton = skeleton_entt;
				skinnedMeshRenderComponent.push_back(skinnedComponent);
			}
		}

		for (int i = 0; i < nodes.size(); ++i) {
			const auto& node = nodes[i]; 
			auto& e_n = entities[i];
			if (!node.name.empty()) {
				registry.emplace<Name>(e_n, Name{ node.name }); 
			}
			//hierarchy
			Hierarchy& child_h = registry.get<Hierarchy>(e_n);
			if (node.id_parent > -1 && node.id_parent < nodes.size()) {
				auto& p_e = entities[node.id_parent]; 
				Hierarchy& parent_h = registry.get<Hierarchy>(p_e); 
				child_h.parent = p_e;
				auto it = std::find(parent_h.children.begin(),
					parent_h.children.end(),
					e_n); 
				if (it == parent_h.children.end()) {
					parent_h.children.emplace_back(e_n); 
				}
			}
			else {
				child_h.parent = root;
				auto it = std::find(h_root.children.begin(),h_root.children.end(),e_n);
				if (it == h_root.children.end()) {
					h_root.children.emplace_back(e_n); 
				}
			}
			//transform
			registry.emplace<Transform>(e_n, Transform{
				node.transform.pos,
				node.transform.quaternion,
				node.transform.scale
				}); 
			//Mesh
			for (const int localMeshId : node.meshIds) {
				if (localMeshId < 0 || localMeshId >= (int)meshLocalToGlobal.size()) {
					Logger::TraceLog(LOG_WARNING, std::format("[GLTF IMPORT] Invalid local mesh : {}", localMeshId));
					continue;
				}
				const uint32_t meshId = meshLocalToGlobal[localMeshId];
				if (!asset.IsValidMesh(meshId)) {
					Logger::TraceLog(LOG_WARNING, std::format("[GLTF IMPORT] Invalid mesh : {}", meshId));
					continue;
				}
				//create new node
				entt::entity e_mesh = registry.create();
				registry.emplace<Name>(e_mesh,Name{std::to_string(meshId)});
				registry.emplace<Transform>(e_mesh,Transform{});
				registry.emplace<MeshFilter>(e_mesh, MeshFilter{ (uint32_t)meshId });
				auto& mesh = asset.GetMesh(meshId);
				registry.emplace<BoxCollider3D>(e_mesh, BoxCollider3D{ mesh.Bounds()});
				// Per-primitive glTF material; fallback when the primitive had none.
				uint32_t meshMaterialId = materialId;
				if (localMeshId < (int)model.meshMaterials.size()) {
					const int localMat = model.meshMaterials[localMeshId];
					if (localMat >= 0 && localMat < (int)materialLocalToGlobal.size()) {
						meshMaterialId = materialLocalToGlobal[localMat];
					}
				}
				registry.emplace<MeshRenderer>(e_mesh, MeshRenderer{ meshMaterialId });
				registry.emplace<Hierarchy>(e_mesh, Hierarchy{ e_n,{} });
				if (node.skinId >= 0 && node.skinId < skinnedMeshRenderComponent.size()) {
					if (skinnedMeshRenderComponent[node.skinId].skeleton != entt::null) {
						registry.emplace<SkinnedMeshRenderer>(e_mesh, skinnedMeshRenderComponent[node.skinId]);
					}
				}
				auto it = std::find(child_h.children.begin(), child_h.children.end(), e_mesh);
				if (it == child_h.children.end()) {
					child_h.children.emplace_back(e_mesh);
				}
			}
		}
		//Read animation clip 
		if (!model.animations.empty()) {
			Animator animator;
			animator.clips.reserve(model.animations.size());
			for (const GLTFAnimationClip& srcClip : model.animations) {
				AnimationClip clip;
				clip.name = srcClip.name;
				clip.duration = srcClip.duration;
				clip.channels.reserve(srcClip.channels.size());
				for (const GLTFAnimChannel& src : srcClip.channels) {
					if (src.node < 0 || src.node >= (int)entities.size()) {
						continue; // importer already warned about unresolved targets
					}
					AnimationChannel ch;
					ch.target = entities[src.node];
					switch (src.path) {
					case GLTFAnimChannel::Path::Translation: ch.path = AnimationChannel::Path::Translation; break;
					case GLTFAnimChannel::Path::Rotation:    ch.path = AnimationChannel::Path::Rotation;    break;
					case GLTFAnimChannel::Path::Scale:       ch.path = AnimationChannel::Path::Scale;       break;
					}
					ch.interp = (src.interp == GLTFAnimChannel::Interp::Step)
						? AnimationChannel::Interp::Step : AnimationChannel::Interp::Linear;
					ch.times = src.times;
					ch.vec3Keys = src.vec3Keys;
					ch.quatKeys = src.quatKeys;
					clip.channels.push_back(std::move(ch));
				}
				if (!clip.channels.empty()) {
					animator.clips.push_back(std::move(clip));
				}
			}
			if (!animator.clips.empty()) {
				registry.emplace<Animator>(root, std::move(animator));
			}
		}
	}
}
#endif // _IMPORT_MODEL_ASSET_HELPER_HPP_!
