#include "engine/materials/WireframeMaterial.hpp"

namespace Long {

	static raylib::Vector4 ToVec4(raylib::Color c) {
		return raylib::Vector4(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
	}

	WireframeMaterial::WireframeMaterial(uint32_t shaderId,
		raylib::Color lineColor, raylib::Color faceColor, float thickness) {
		SetShaderId(shaderId);
		SetLineColor(lineColor);
		SetFaceColor(faceColor);
		SetThickness(thickness);
	}

	void WireframeMaterial::SetLineColor(raylib::Color color) {
		SetUniform("u_lineColor", ToVec4(color));
	}

	void WireframeMaterial::SetFaceColor(raylib::Color color) {
		SetUniform("u_faceColor", ToVec4(color));
	}

	void WireframeMaterial::SetThickness(float thickness) {
		SetUniform("u_thickness", thickness);
	}


} // namespace Long
