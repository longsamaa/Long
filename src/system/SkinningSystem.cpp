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
				skel.jointMatrices[j] = jointWorld.Multiply(skel.inverseBind[j]);
			}
			++skel.version;
		}
	}
}
