#pragma once
#ifndef _WIREFRAME_MATERIAL_HPP_
#define _WIREFRAME_MATERIAL_HPP_

#include "engine/Material.hpp"

namespace Long {

	// Draws box-edge wireframe using the mesh UVs (see wireframe.frag): a line
	// color around each face plus a fill color inside. Good for tile/grid looks.
	class WireframeMaterial : public BaseMaterial {
	public:
		explicit WireframeMaterial(uint32_t shaderId,
			raylib::Color lineColor = raylib::Color::Black(),
			raylib::Color faceColor = raylib::Color::White(),
			float thickness = 0.03f);
		void SetLineColor(raylib::Color color);
		void SetFaceColor(raylib::Color color);
		void SetThickness(float thickness);
		raylib::Material& Apply(raylib::Shader& shader) override;
	};

} // namespace Long

#endif // !_WIREFRAME_MATERIAL_HPP_
