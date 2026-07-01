#pragma once
#ifndef _VISIBILITY_SYSTEM_HPP_
#define _VISIBILITY_SYSTEM_HPP_
#include <entt/entt.hpp>
#include <vector>
namespace Long {
	class FrustumCulling;
	class IVisibility {
	public:
		virtual ~IVisibility() = default;
		virtual void gatherVisible(entt::registry& registry,
			const FrustumCulling* frustum,
			std::vector<entt::entity>& out,
			uint32_t& outCulledCount) = 0;
	};
	class LinearVisibility : public IVisibility {
	public:
		void gatherVisible(entt::registry& registry,
			const FrustumCulling* frustum,
			std::vector<entt::entity>& out,
			uint32_t& outCulledCount) override;
	};
}
#endif // !_VISIBILITY_SYSTEM_HPP_