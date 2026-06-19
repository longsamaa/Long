#pragma once
#ifndef _IRENDER_PASS_HPP_
#define _IRENDER_PASS_HPP_
namespace Long {
	class IRenderPass {
	public:
		virtual ~IRenderPass() = default;
		bool isEnabled() const { return isEnabled; }
	protected:
		bool isEnabled{ true };
	};
}
#endif // !_IRENDER_PASS_HPP_