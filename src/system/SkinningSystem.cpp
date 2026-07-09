#include "SkinningSystem.hpp"
#include "core/Components.hpp"
#include <raylib-cpp.hpp>

namespace Long {
	void SkinningSystem(entt::registry& registry) {
		auto view = registry.view<Skeleton>();
		for (entt::entity e : view) {
			Skeleton& skel = view.get<Skeleton>(e);
			const size_t n = skel.joints.size();
			if (skel.jointMatrices.size() != n) {
				skel.jointMatrices.assign(n, raylib::Matrix::Identity());
			}
			for (size_t j = 0; j < n; ++j) {
				entt::entity joint = skel.joints[j];
				raylib::Matrix jointWorld = raylib::Matrix::Identity();
				if (joint != entt::null && registry.valid(joint)) {
					if (const MatrixTransform* mt = registry.try_get<MatrixTransform>(joint)) {
						jointWorld = mt->world_matrix;
					}
				}
				// Skin matrix = jointWorld * inverseBind: inverseBind takes the
				// vertex from mesh-local into joint-local space, jointWorld puts it
				// back with the (animated) joint pose. At bind pose this is
				// identity, so vertices stay in mesh-local space and matModel
				// (the mesh node's world transform) places them in the world.
				skel.jointMatrices[j] = jointWorld.Multiply(skel.inverseBind[j]);
			}
			++skel.version;
		}
	}
}
