#pragma once
#ifndef _IRENDER_PASS_HPP_
#define _IRENDER_PASS_HPP_
#include <unordered_map>
#include <raylib-cpp.hpp>
#include "RenderContext.hpp"
namespace Long {
	class IRenderPass {
	public:
		virtual ~IRenderPass() = default;
		virtual void execute(RenderContext& ctx) = 0; 
		bool disable() { _isEnabled = false; }
		bool enable() { _isEnabled = true; }
		bool isEnabled() const { return _isEnabled; }
		uint32_t getLoc(const std::string& name, raylib::Shader& shader);
	protected:
		bool _isEnabled{ true };
		std::unordered_map<std::string, uint32_t> m_location; 
	
	};
}
#endif // !_IRENDER_PASS_HPP_