#include "FrustumCulling.hpp"
#include "rcamera.h"
namespace Long {
	void FrustumCulling::setCamera(BaseCamera* _camera)
	{
		camera = _camera; 
	}
	void FrustumCulling::update()
	{
		buildPlane(); 
	}
	void FrustumCulling::buildPlane()
	{
		if (!camera) return;
		const raylib::Camera3D& cam = camera->Raw();
		raylib::Matrix view_matrix = cam.GetMatrix();
		const float aspect = (float)::GetScreenWidth() / (float)::GetScreenHeight();
		raylib::Matrix proj_matrix;
		if (cam.projection == CAMERA_PERSPECTIVE) {
			proj_matrix = MatrixPerspective(camera->Raw().GetFovy() * DEG2RAD, aspect,
				camera->Near(), camera->Far());
		}
		else { // CAMERA_ORTHOGRAPHIC: fovy is the full vertical world height
			double top = camera->Raw().GetFovy() / 2.0;
			double right = top * aspect;
			proj_matrix = MatrixOrtho(-right, right, -top, top,
				camera->Near(), camera->Far());
		}
		raylib::Matrix m = view_matrix.Multiply(proj_matrix); 
		auto setPlane = [&](int i, float a, float b, float c, float d) {
			raylib::Vector3 n{ a, b, c };
			float len = n.Length();
			if (len > 0.0f) { n = n.Scale(1.0f / len); d /= len; }
			m_planes[i].normal = n;
			m_planes[i].d = d;
		};
		// Left, Right, Bottom, Top, Near, Far.
		setPlane(0, m.m3 + m.m0, m.m7 + m.m4, m.m11 + m.m8,  m.m15 + m.m12);
		setPlane(1, m.m3 - m.m0, m.m7 - m.m4, m.m11 - m.m8,  m.m15 - m.m12);
		setPlane(2, m.m3 + m.m1, m.m7 + m.m5, m.m11 + m.m9,  m.m15 + m.m13);
		setPlane(3, m.m3 - m.m1, m.m7 - m.m5, m.m11 - m.m9,  m.m15 - m.m13);
		setPlane(4, m.m3 + m.m2, m.m7 + m.m6, m.m11 + m.m10, m.m15 + m.m14);
		setPlane(5, m.m3 - m.m2, m.m7 - m.m6, m.m11 - m.m10, m.m15 - m.m14);
	}

	bool FrustumCulling::isVisible(const raylib::Vector3& min, const raylib::Vector3& max) const {
		for (const Plane& pl : m_planes) {
			raylib::Vector3 p{
				pl.normal.x >= 0.0f ? max.x : min.x,
				pl.normal.y >= 0.0f ? max.y : min.y,
				pl.normal.z >= 0.0f ? max.z : min.z,
			};
			if (pl.normal.DotProduct(p) + pl.d < 0.0f) {
				return false;
			}
		}
		return true;
	}
}