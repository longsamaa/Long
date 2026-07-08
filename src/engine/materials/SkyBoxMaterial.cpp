#include "SkyBoxMaterial.hpp"
namespace Long {
	SkyboxMaterial::SkyboxMaterial(uint32_t shaderId, raylib::Vector3 topColor, raylib::Vector3 botColor, float gradientSharpness)
	{
		SetShaderId(shaderId);
		SetColor(topColor, botColor, gradientSharpness);
	}
	void SkyboxMaterial::SetColor(raylib::Vector3 topColor, raylib::Vector3 botColor, float gradientSharpness)
	{
		SetUniform("u_topColor", topColor);
		SetUniform("u_bottomColor", botColor);
		SetUniform("u_sharpness", gradientSharpness);
	}
}