#pragma once
#ifndef _IRENDER_PASS_HPP_
#define _IRENDER_PASS_HPP_
#include "RenderContext.hpp"
namespace Long {
	class IRenderPass {
	public:
		virtual ~IRenderPass() = default;
		virtual void execute(RenderContext& ctx) = 0; 
		bool disable() { _isEnabled = false; }
		bool enable() { _isEnabled = true; }
		bool isEnabled() const { return _isEnabled; }
	protected:
		bool _isEnabled{ true };
	};
}
#endif // !_IRENDER_PASS_HPP_