#include "engine/camera.hpp"

namespace Long {

EditorCamera::EditorCamera() {
    // Look down at the origin from above and behind -> tilted top-down view.
    m_camera.position   = {10.0f, 12.0f, 10.0f};
    m_camera.target     = {0.0f, 0.0f, 0.0f};
    m_camera.up         = {0.0f, 1.0f, 0.0f};
    m_camera.fovy       = 45.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;
}

} // namespace Long
