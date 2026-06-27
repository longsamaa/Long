#include "IRenderPass.hpp"
#include <raylib-cpp.hpp>
namespace Long {
	uint32_t IRenderPass::getLoc(const std::string& name, raylib::Shader& shader)
	{
		auto it = m_location.find(name); 
		if (it == m_location.end()) {
			uint32_t loc = shader.GetLocation(name);
			m_location[name] = loc;
			return loc;
		}
		return m_location[name]; 
	}
}

