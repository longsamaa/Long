#pragma once
#ifndef _SKINNING_SYSTEM_HPP_
#define _SKINNING_SYSTEM_HPP_
#include <entt/entt.hpp>
namespace Long {
	// Fills each Skeleton's jointMatrices from its joints' world transforms:
	//   jointMatrices[j] = jointWorld[j] * inverseBind[j]
	// which is exactly what the skinning shader multiplies bind-pose vertices by.
	// Run AFTER TransformSystem (needs joint MatrixTransform.world_matrix) and
	// BEFORE rendering. Static bind pose today; animation just changes the joint
	// Transforms upstream, and this recomputes automatically.
	void SkinningSystem(entt::registry& registry);
}
#endif // !_SKINNING_SYSTEM_HPP_
