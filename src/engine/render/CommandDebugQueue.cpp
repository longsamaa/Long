#include "CommandDebugQueue.hpp"
#include <raylib-cpp.hpp>
#include <cmath>

namespace Long {
	void CommandDebugQueue::Submit(const DebugCommand& cmd)
	{
		m_commands.emplace_back(cmd);
	}
	const std::vector<DebugCommand>& CommandDebugQueue::getGetCommands()
	{
		return m_commands;
	}

	void CommandDebugQueue::AddLine(const raylib::Vector3& a, const raylib::Vector3& b,
		const raylib::Color& c)
	{
		m_lines.push_back({ a.x, a.y, a.z, c.r, c.g, c.b, c.a });
		m_lines.push_back({ b.x, b.y, b.z, c.r, c.g, c.b, c.a });
	}

	// Line circle around `center` in the plane perpendicular to `axis`
	// (0 = X, 1 = Y, 2 = Z). Debug stand-in for the old solid spheres.
	void CommandDebugQueue::AddCircle(const raylib::Vector3& center, float radius,
		int axis, const raylib::Color& c)
	{
		constexpr int kSegments = 16;
		raylib::Vector3 prev{};
		for (int s = 0; s <= kSegments; ++s) {
			float t = (float)s / kSegments * 2.0f * PI;
			float ca = cosf(t) * radius;
			float sa = sinf(t) * radius;
			raylib::Vector3 p = center;
			if (axis == 0) { p.y += ca; p.z += sa; }
			else if (axis == 1) { p.x += ca; p.z += sa; }
			else { p.x += ca; p.y += sa; }
			if (s > 0) {
				AddLine(prev, p, c);
			}
			prev = p;
		}
	}

	// Wireframe AABB: 4 bottom edges, 4 top edges, 4 verticals.
	void CommandDebugQueue::AddBox(const raylib::Vector3& min, const raylib::Vector3& max,
		const raylib::Color& c)
	{
		const raylib::Vector3 b0{ min.x, min.y, min.z }, b1{ max.x, min.y, min.z };
		const raylib::Vector3 b2{ max.x, min.y, max.z }, b3{ min.x, min.y, max.z };
		const raylib::Vector3 t0{ min.x, max.y, min.z }, t1{ max.x, max.y, min.z };
		const raylib::Vector3 t2{ max.x, max.y, max.z }, t3{ min.x, max.y, max.z };
		AddLine(b0, b1, c); AddLine(b1, b2, c); AddLine(b2, b3, c); AddLine(b3, b0, c);
		AddLine(t0, t1, c); AddLine(t1, t2, c); AddLine(t2, t3, c); AddLine(t3, t0, c);
		AddLine(b0, t0, c); AddLine(b1, t1, c); AddLine(b2, t2, c); AddLine(b3, t3, c);
	}

	void CommandDebugQueue::BuildLines(RenderStats& stats)
	{
		m_lines.clear();
		for (auto& cmd : m_commands) {
			std::visit([&](auto&& v) {
				using T = std::decay_t<decltype(v)>;
				if constexpr (std::is_same_v<T, GridCommand>) {
					// Mirrors raylib's DrawGrid: (slices+1) lines each way on the
					// XZ plane, darker pair through the origin.
					const int half = (int)v.slices / 2;
					const float ext = half * v.spacing;
					for (int i = -half; i <= half; ++i) {
						raylib::Color c = (i == 0) ? raylib::Color{ 90, 90, 90, 255 }
						: raylib::Color{ 120, 120, 120, 255 };
						float d = i * v.spacing;
						AddLine({ d, 0, -ext }, { d, 0, ext }, c);
						AddLine({ -ext, 0, d }, { ext, 0, d }, c);
					}
				}
				else if constexpr (std::is_same_v<T, CameraHelperCommand>) {
					const raylib::Color g = raylib::Color::Green();
					// Body marker: 3 axis circles instead of a solid sphere.
					AddCircle(v.pos, 0.3f, 0, raylib::Color::Red());
					AddCircle(v.pos, 0.3f, 1, raylib::Color::Red());
					AddCircle(v.pos, 0.3f, 2, raylib::Color::Red());
					// Frustum edges.
					AddLine(v.pos, v.n_tl, g); AddLine(v.pos, v.n_tr, g);
					AddLine(v.pos, v.n_bl, g); AddLine(v.pos, v.n_br, g);
					AddLine(v.n_tl, v.n_tr, g); AddLine(v.n_tr, v.n_br, g);
					AddLine(v.n_br, v.n_bl, g); AddLine(v.n_bl, v.n_tl, g);
					AddLine(v.n_tl, v.f_tl, g); AddLine(v.n_tr, v.f_tr, g);
					AddLine(v.n_br, v.f_br, g); AddLine(v.n_bl, v.f_bl, g);
					AddLine(v.f_tl, v.f_tr, g); AddLine(v.f_tr, v.f_br, g);
					AddLine(v.f_br, v.f_bl, g); AddLine(v.f_bl, v.f_tl, g);
				}
				else if constexpr (std::is_same_v<T, BoxHelperCommand>) {
					AddBox(v.min, v.max, v.color);
				}
				else if constexpr (std::is_same_v<T, LightHelperCommand>) {
					raylib::Vector3 dir = raylib::Vector3(v.direction).Normalize();
					raylib::Vector3 end = raylib::Vector3(v.origin).Add(dir.Scale(v.length));
					AddCircle(v.origin, 0.2f, 0, v.color);
					AddCircle(v.origin, 0.2f, 1, v.color);
					AddCircle(v.origin, 0.2f, 2, v.color);
					raylib::Vector3 ref = (fabsf(dir.y) < 0.9f)
						? raylib::Vector3{ 0, 1, 0 } : raylib::Vector3{ 1, 0, 0 };
					raylib::Vector3 u = dir.CrossProduct(ref).Normalize().Scale(0.5f);
					raylib::Vector3 w = dir.CrossProduct(u).Normalize().Scale(0.5f);
					AddLine(v.origin, end, v.color);
					for (auto off : { u, u.Scale(-1.0f), w, w.Scale(-1.0f) }) {
						raylib::Vector3 o = raylib::Vector3(v.origin).Add(off);
						AddLine(o, o.Add(dir.Scale(v.length)), v.color);
					}
				}
				}, cmd);
		}
		stats.debugLineCount = (uint32_t)(m_lines.size() / 2);
	}

	void CommandDebugQueue::Clear()
	{
		m_commands.clear();
	}
}