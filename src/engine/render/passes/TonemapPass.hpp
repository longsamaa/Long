#pragma once
#ifndef _TONEMAP_PASS_HPP_
#define _TONEMAP_PASS_HPP_
#include "engine/render/IRenderPass.hpp"
namespace Long {
	// Final pass: tonemaps the composited HDR image to LDR and (optionally) FXAAs it
	// in the same shader, then draws straight to the screen. fxaaEnabled toggles the
	// antialiasing at runtime like a game setting (off = cheaper, just tonemap).
	class TonemapPass : public IRenderPass {
	public:
		void execute(RenderContext& ctx) override;

		float u_exposure{ 0.4f }; // overall brightness before tonemapping
		bool fxaaEnabled{ true };
	private:
		uint32_t m_shaderId{ UINT32_MAX };
	};

}
#endif // !_TONEMAP_PASS_HPP_
